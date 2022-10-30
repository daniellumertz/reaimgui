/* ReaImGui: ReaScript binding for Dear ImGui
 * Copyright (C) 2021-2022  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cocoa_window.hpp"

#include "cocoa_events.hpp"
#include "cocoa_inject.hpp"
#include "cocoa_inputview.hpp"
#include "context.hpp"
#include "platform.hpp"
#include "renderer.hpp"

#include <imgui/imgui_internal.h>
#include <reaper_plugin_secrets.h>

static_assert(__has_feature(objc_arc),
  "This file must be built with automatic reference counting enabled.");

@interface SWELLWindowOverride : NSWindow
- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen;
@end

@implementation SWELLWindowOverride
- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen
{
  if(!CocoaInject::isTarget(self))
    return [super constrainFrameRect:frameRect toScreen:screen];

  // allow setPosition to move the window outside of the woring area
  return frameRect;
}
@end

CocoaWindow::CocoaWindow(ImGuiViewport *viewport, DockerHost *dockerHost)
  : Window { viewport, dockerHost }
{
}

CocoaWindow::~CocoaWindow()
{
}

void CocoaWindow::create()
{
  createSwellDialog();
  m_view = (__bridge NSView *)m_hwnd.get(); // SWELL_hwndChild inherits from NSView
  m_inputView = [[InputView alloc] initWithWindow:this];

  NSWindow *window { [m_view window] };
  m_defaultStyleMask = [window styleMask];
  m_previousScale = m_viewport->DpiScale;
  m_previousFlags = ~m_viewport->Flags; // mark all as modified
  // imgui calls update() before show(), it will apply the flags

  m_renderer = m_ctx->rendererFactory()->create(this);

  if(!isDocked()) {
    CocoaInject::inject([SWELLWindowOverride class], window);

    // enable transparency
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor clearColor]]; // required when decorations are enabled
    [m_view setWantsLayer:YES]; // required to be transparent before resizing
  }

  static __weak EventHandler *g_handler;
  if(g_handler)
    m_eventHandler = g_handler;
  else
    g_handler = m_eventHandler = [[EventHandler alloc] init];
}

void CocoaWindow::show()
{
  Window::show();
  [m_eventHandler watchView:m_view];
}

void CocoaWindow::setPosition(ImVec2 pos)
{
  Platform::scalePosition(&pos, true);

  NSWindow *window { [m_view window] };
  const NSRect &content { [window contentRectForFrameRect:[window frame]] };
  const CGFloat titleBarHeight { [window frame].size.height - content.size.height };
  [window setFrameTopLeftPoint:NSMakePoint(pos.x, pos.y + titleBarHeight)];
}

void CocoaWindow::setSize(const ImVec2 size)
{
  // most scripts expect y=0 to be the top of the window
  NSWindow *window { [m_view window] };
  [window setContentSize:NSMakeSize(size.x, size.y)];
  setPosition(m_viewport->Pos); // preserve y position from the top
}

void CocoaWindow::setFocus()
{
  NSWindow *window { [m_view window] };
  [window makeKeyAndOrderFront:nil];
  [window makeFirstResponder:m_inputView];
}

bool CocoaWindow::hasFocus() const
{
  // m_hwnd temporarily has focus between ShowWindow and WM_SETFOCUS
  // returning true here in this case is important, otherwise
  // Context::updateFocus would misdetect no active windows when it's called
  // from EventHandler::windowDidResignKey
  HWND focus { GetFocus() };
  return focus == (__bridge HWND)m_inputView || focus == m_hwnd.get();
}

void CocoaWindow::setTitle(const char *title)
{
  SetWindowText(m_hwnd.get(), title);
}

void CocoaWindow::setAlpha(const float alpha)
{
  NSWindow *window { [m_view window] };
  [window setAlphaValue:alpha];
}

void CocoaWindow::update()
{
  // restore keyboard focus to the input view when switching to our docker tab
  static bool no_wm_setfocus { atof(GetAppVersion()) < 6.53 };
  if(no_wm_setfocus && GetFocus() == m_hwnd.get())
    setFocus();

  if(m_previousScale != m_viewport->DpiScale) {
    // resize macOS's GL objects when DPI changes (eg. moving to another screen)
    // NSViewFrameDidChangeNotification or WM_SIZE aren't sent
    m_renderer->setSize(m_viewport->Size);
    m_previousScale = m_viewport->DpiScale;
  }

  if(isDocked())
    return;

  const ImGuiViewportFlags diff { m_previousFlags ^ m_viewport->Flags };
  m_previousFlags = m_viewport->Flags;

  NSWindow *window { [m_view window] };

  if(diff & ImGuiViewportFlags_NoDecoration) {
    if(m_viewport->Flags & ImGuiViewportFlags_NoDecoration) {
      DetachWindowTopmostButton(m_hwnd.get(), false);
      [window setStyleMask:NSWindowStyleMaskBorderless];
    }
    else {
      [window setStyleMask:m_defaultStyleMask];
      AttachWindowTopmostButton(m_hwnd.get());
      // ask dear imgui to call setText again
      static_cast<ImGuiViewportP *>(m_viewport)->LastNameHash = 0;
    }
  }

  if(diff & ImGuiViewportFlags_TopMost) {
    // Using SWELL_SetWindowWantRaiseAmt instead of [NSWindow setLevel]
    // to automaticaly restore the level when the application regains focus.
    //
    // 0=normal/1=topmost as observed via a breakpoint when clicking on
    // AttachWindowTopmostButton's thumbstack button.
    const bool topmost { (m_viewport->Flags & ImGuiViewportFlags_TopMost) != 0 };
    SWELL_SetWindowWantRaiseAmt(m_hwnd.get(), topmost);
  }

  // disable shadows under the window when WindowFlags_NoBackground is set
  // (shadows wouldn't be updated along with the contents until next resize)
  if(diff & ImGuiViewportFlags_NoRendererClear) {
    const bool opaque { (m_viewport->Flags & ImGuiViewportFlags_NoRendererClear) != 0 };
    [window setHasShadow:opaque];
  }
}

float CocoaWindow::scaleFactor() const
{
  return [[m_view window] backingScaleFactor];
}

void CocoaWindow::setIME(ImGuiPlatformImeData *data)
{
  if(!data->WantVisible) {
    NSTextInputContext *inputContext { [m_inputView inputContext] };
    [inputContext discardMarkedText];
    [inputContext invalidateCharacterCoordinates];
  }

  ImVec2 pos { data->InputPos };
  Platform::scalePosition(&pos, true);
  pos.y -= data->InputLineHeight;

  [m_inputView setImePosition:NSMakePoint(pos.x, pos.y)];
}

std::optional<LRESULT> CocoaWindow::handleMessage
  (const unsigned int msg, WPARAM wParam, LPARAM)
{
  switch(msg) {
  case WM_SETFOCUS: // REAPER v6.53+
    // Redirect focus to the input view after m_view gets it.
    // WM_SETFOCUS is sent from becomeFirstResponder,
    // m_view will gain focus right after this returns.
    //
    // Both makeFirstResponder and making the window key are required
    // for receiving key events right away without a click
    if(!(m_viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing) ||
        IsWindowVisible(m_hwnd.get()))
      dispatch_async(dispatch_get_main_queue(), ^{ setFocus(); });
    break;
  }

  return std::nullopt;
}

int CocoaWindow::handleAccelerator(MSG *msg)
{
  [[m_view window] sendEvent:[NSApp currentEvent]];
  return Accel::EatKeystroke;
}

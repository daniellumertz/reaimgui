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

#include "gdk_window.hpp"

#include "context.hpp"
#include "opengl_renderer.hpp"
#include "platform.hpp"

#include <gtk/gtk.h>

#include <reaper_plugin_functions.h>
#include <swell/swell.h>
#include <WDL/wdltypes.h>

static GdkWindow *getOSWindow(HWND hwnd)
{
  static bool hasOSWindow { atof(GetAppVersion()) >= 6.57 };

  return static_cast<GdkWindow *>(
    hasOSWindow ? SWELL_GetOSWindow(hwnd, "GdkWindow")
                : *(reinterpret_cast<char **>(hwnd) + 1)
  );
}

template<typename T>
static T *currentEvent(const int expectedType)
{
  void *event { SWELL_GetOSEvent("GdkEvent") };
  if(event && static_cast<GdkEvent *>(event)->type == expectedType)
    return static_cast<T *>(event);
  else
    return nullptr;
}

GDKWindow::GDKWindow(ImGuiViewport *viewport, DockerHost *dockerHost)
  : Window { viewport, dockerHost }, m_ime { nullptr }, m_imeOpen { false }
{
}

void GDKWindow::create()
{
  createSwellDialog();
  SetProp(m_hwnd.get(), "SWELLGdkAlphaChannel", reinterpret_cast<HANDLE>(1));
  SetWindowLongPtr(m_hwnd.get(), GWL_EXSTYLE, WS_EX_ACCEPTFILES);

  // WS_CHILD does gdk_window_set_override_redirect(true)
  // SWELL only supports setting WS_CHILD before ShowWindow
  if(m_viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    SetWindowLongPtr(m_hwnd.get(), GWL_STYLE, WS_CHILD);

  m_previousFlags = ~m_viewport->Flags; // update will be called before show
}

GDKWindow::~GDKWindow()
{
  if(m_ime)
    g_object_unref(m_ime);
}

GdkWindow *GDKWindow::getOSWindow() const
{
  return ::getOSWindow(m_hwnd.get());
}

static void imeCommit(GtkIMContext *, gchar *input, gpointer data)
{
  Context *ctx { reinterpret_cast<Context *>(data) };
  while(*input) {
    ctx->charInput(g_utf8_get_char(input));
    input = g_utf8_next_char(input);
  }
}

void GDKWindow::imePreeditStart(GtkIMContext *, gpointer data)
{
  reinterpret_cast<GDKWindow *>(data)->m_imeOpen = true;
}

void GDKWindow::imePreeditEnd(GtkIMContext *, gpointer data)
{
  reinterpret_cast<GDKWindow *>(data)->m_imeOpen = false;
}

void GDKWindow::initIME()
{
  m_ime = gtk_im_multicontext_new();
  gtk_im_context_set_use_preedit(m_ime, true);
  gtk_im_context_focus_in(m_ime);
  g_signal_connect(m_ime, "commit", G_CALLBACK(imeCommit), m_ctx);
  g_signal_connect(m_ime, "preedit-start", G_CALLBACK(imePreeditStart), this);
  g_signal_connect(m_ime, "preedit-end",   G_CALLBACK(imePreeditEnd),   this);
}

void GDKWindow::show()
{
  Window::show();
  initIME();
  m_renderer = m_ctx->rendererFactory()->create(this);
}

void GDKWindow::setPosition(ImVec2 pos)
{
  Platform::scalePosition(&pos, true);
  SetWindowPos(m_hwnd.get(), nullptr, pos.x, pos.y, 0, 0,
    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void GDKWindow::setSize(const ImVec2 size)
{
  SetWindowPos(m_hwnd.get(), nullptr, 0, 0,
    size.x * scaleFactor(), size.y * scaleFactor(),
    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}

void GDKWindow::setTitle(const char *title)
{
  SetWindowText(m_hwnd.get(), title);
}

void GDKWindow::setAlpha(const float alpha)
{
  gdk_window_set_opacity(getOSWindow(), alpha);
}

void GDKWindow::update()
{
  if(GetFocus() == m_hwnd.get())
    SWELL_SetClassName(m_hwnd.get(), getSwellClass());

  if(isDocked())
    return;

  const ImGuiViewportFlags diff { m_previousFlags ^ m_viewport->Flags };
  m_previousFlags = m_viewport->Flags;

  if(diff & ImGuiViewportFlags_NoDecoration) {
    auto style { GetWindowLongPtr(m_hwnd.get(), GWL_STYLE) };

    if(m_viewport->Flags & ImGuiViewportFlags_NoDecoration)
      style &= ~WS_CAPTION;
    else
      style |= WS_CAPTION;

    SetWindowLongPtr(m_hwnd.get(), GWL_STYLE, style);

    // SetWindowLongPtr hides the window
    // it sets an internal "need show" flag that's used by SetWindowPos
    if(getOSWindow()) {
      setPosition(m_viewport->Pos);
      setSize(m_viewport->Size);
    }
  }

  if(diff & ImGuiViewportFlags_TopMost) {
    if(m_viewport->Flags & ImGuiViewportFlags_TopMost)
      SWELL_SetWindowLevel(m_hwnd.get(), 1);
    else
      SWELL_SetWindowLevel(m_hwnd.get(), 0);
  }
}

void GDKWindow::render(void *)
{
  if(m_needTexUpload) {
    m_renderer->uploadFontTex(m_fontAtlas);
    m_needTexUpload = false;
  }

  m_renderer->render(m_viewport);
}

float GDKWindow::globalScaleFactor()
{
  static float scale { SWELL_GetScaling256() / 256.f };
  return scale;
}

void GDKWindow::setIME(ImGuiPlatformImeData *data)
{
  if(!data->WantVisible)
    gtk_im_context_reset(m_ime);

  // cannot use m_viewport->Pos when docked
  // (IME cursor location must be relative to the dock host window)
  HWND container { m_hwnd.get() };
  while(!::getOSWindow(container))
    container = GetParent(container);
  RECT containerPos;
  if(container)
    GetWindowRect(container, &containerPos);
  else {
    containerPos.left = m_viewport->Pos.x;
    containerPos.right = m_viewport->Pos.y;
  }

  GdkRectangle area;
  area.x = data->InputPos.x - containerPos.left;
  area.y = data->InputPos.y - containerPos.top;
  area.width = 0;
  area.height = data->InputLineHeight;
  gtk_im_context_set_cursor_location(m_ime, &area);
}

static unsigned int unmangleSwellChar(WPARAM wParam, LPARAM lParam)
{
  // Trying to guess the character to print from SWELL's event data.
  // Matching the behavior of OnEditKeyDown from swell-wnd-generic.cpp

  if(lParam & (FCONTROL | FALT | FLWIN) || wParam < 32)
    return 0;

  if(wParam >= 'A' && wParam <= 'Z') {
    // This does not support caps lock.
    if(!(lParam & FSHIFT))
      wParam += 'a' - 'A';
  }
  else if(wParam >= VK_NUMPAD0 && wParam <= VK_DIVIDE) {
    if(wParam <= VK_NUMPAD9)
      wParam += '0' - VK_NUMPAD0;
    else
      wParam += '*' - VK_MULTIPLY;
  }
  else if(lParam & FVIRTKEY && (wParam < '0' || wParam > '9') && wParam != VK_SPACE)
    return 0; // virtual keys that aren't letters or numbers aren't printable

  return wParam;
}

static ImGuiMouseButton translateButton(const GdkEventButton *event)
{
  switch(event->button) {
  case 1: case 2: case 3: return event->button - 1;
  // 4/5/6/7 are scroll/thumb wheels, doesn't trigger WM_LBUTTON messages
  case 8: case 9:         return event->button - 5;
  default:                return ImGuiMouseButton_Left;
  }
}

std::optional<LRESULT> GDKWindow::handleMessage
  (const unsigned int msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg) {
  case WM_DROPFILES: {
    HDROP drop { reinterpret_cast<HDROP>(wParam) };
    m_ctx->beginDrag(drop);
    DragFinish(drop);
    m_ctx->endDrag(true);
    return 0;
  }
  case WM_LBUTTONDOWN: // for supporting thumb buttons
  case WM_LBUTTONUP: //   SWELL treats thumb buttons as Left
    if(auto *event { currentEvent<GdkEventButton>(GDK_BUTTON_PRESS) })
      mouseDown(translateButton(event));
    else if(auto *event { currentEvent<GdkEventButton>(GDK_BUTTON_RELEASE) })
      mouseUp(translateButton(event));
    else if(!currentEvent<GdkEventButton>(GDK_2BUTTON_PRESS))
      break;  // do default SWELL message handling in Window
    return 0; // eat SWELL message if handled the GDK event
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP:
    keyEvent(wParam, lParam, msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    return 0;
  case WM_SIZE:
  case WM_PAINT:
    if(m_renderer)
      m_renderer->peekMessage(msg);
    break; // continue handling WM_SIZE in Window::proc
  }

  return std::nullopt;
}

static ImGuiKey translateGdkKey(const GdkEventKey *event)
{
  switch(event->keyval) {
  case GDK_KEY_Control_L: return ImGuiKey_LeftCtrl;
  case GDK_KEY_Control_R: return ImGuiKey_RightCtrl;
  case GDK_KEY_Shift_L:   return ImGuiKey_LeftShift;
  case GDK_KEY_Shift_R:   return ImGuiKey_RightShift;
  case GDK_KEY_Alt_L:     return ImGuiKey_LeftAlt;
  case GDK_KEY_Alt_R:     return ImGuiKey_RightAlt;
  case GDK_KEY_Super_L:   return ImGuiKey_LeftSuper;
  case GDK_KEY_Super_R:   return ImGuiKey_RightSuper;
  case GDK_KEY_KP_Enter:  return ImGuiKey_KeypadEnter;
  default:                return ImGuiKey_None;
  }
}

void GDKWindow::keyEvent(WPARAM swellKey, LPARAM lParam, const bool down)
{
  const GdkEventType expectedType { down ? GDK_KEY_PRESS : GDK_KEY_RELEASE };
  auto *gdkEvent { currentEvent<GdkEventKey>(expectedType) };

  struct Modifier { unsigned int vkey; ImGuiKey modkey, ikey; };
  constexpr Modifier modifiers[] {
    { VK_CONTROL, ImGuiKey_ModCtrl,  ImGuiKey_LeftCtrl  },
    { VK_SHIFT,   ImGuiKey_ModShift, ImGuiKey_LeftShift },
    { VK_MENU,    ImGuiKey_ModAlt,   ImGuiKey_LeftAlt   },
    { VK_LWIN,    ImGuiKey_ModSuper, ImGuiKey_LeftSuper },
  };

  for(const auto &modifier : modifiers) {
    if(swellKey != modifier.vkey)
      continue;
    // post key events only when both sides of the modifier have the same state
    if(!!(GetAsyncKeyState(swellKey) & 0x8000) == down) {
      m_ctx->keyInput(modifier.modkey, down);
      if(!gdkEvent)
        m_ctx->keyInput(modifier.ikey, down);
    }
    break;
  }


  if(gdkEvent) {
    gtk_im_context_filter_keypress(m_ime, gdkEvent);

    // filter_keypress seems to always returns true, so we can't accurately
    // tell when a key event was used by the IME and should be eaten.
    // This workaround works for all events except for the initial keydown
    // (preedit begins at the keyup).
    if(m_imeOpen && down)
      return;

    if(ImGuiKey namedKey { translateGdkKey(gdkEvent) }) {
      m_ctx->keyInput(namedKey, down);
      return;
    }
  }
  else if(down)
    m_ctx->charInput(unmangleSwellChar(swellKey, lParam));

  if(swellKey < 256)
    m_ctx->keyInput(swellKey, down);
}

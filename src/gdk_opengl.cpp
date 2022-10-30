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

#include "opengl_renderer.hpp"

#include "errors.hpp"
#include "gdk_window.hpp"

#include <cassert>
#include <epoxy/gl.h>
#include <gtk/gtk.h>
#include <imgui/imgui.h>

struct LICEDeleter {
  void operator()(LICE_IBitmap *bm) { LICE__Destroy(bm); }
};

class GDKOpenGL : public OpenGLRenderer {
public:
  GDKOpenGL(RendererFactory *, Window *);
  ~GDKOpenGL();

  void setSize(ImVec2) override;
  void render(void *) override;
  void swapBuffers(void *) override;

private:
  void initSoftwareBlit();
  void resizeTextures(ImVec2);
  void softwareBlit();

  GdkGLContext *m_gl;
  unsigned int m_tex, m_fbo;

  // for docking
  std::unique_ptr<LICE_IBitmap, LICEDeleter> m_pixels;
  std::shared_ptr<GdkWindow> m_offscreen;
};

class MakeCurrent {
public:
  MakeCurrent(GdkGLContext *gl)
    : m_gl { gl }
  {
    gdk_gl_context_make_current(m_gl);
  }

  ~MakeCurrent()
  {
    gdk_gl_context_clear_current();
  }

private:
  GdkGLContext *m_gl;
};

std::unique_ptr<Renderer> RendererFactory::create(Window *window)
{
  return std::make_unique<GDKOpenGL>(this, window);
}

// GdkGLContext cannot share ressources: they're already shared with the
// window's paint context (which itself isn't shared with anything).
GDKOpenGL::GDKOpenGL(RendererFactory *factory, Window *window)
  : OpenGLRenderer(factory, window, false)
{
  GdkWindow *osWindow;

  if(m_window->isDocked()) {
    initSoftwareBlit();
    osWindow = m_offscreen.get();
  }
  else
    osWindow = static_cast<GDKWindow *>(m_window)->getOSWindow();

  if(!osWindow || static_cast<void *>(osWindow) == m_window->nativeHandle())
    throw backend_error { "headless SWELL is not supported" };

  GError *error {};
  m_gl = gdk_window_create_gl_context(osWindow, &error);
  if(error) {
    const backend_error ex { error->message };
    g_clear_error(&error);
    assert(!m_gl);
    throw ex;
  }

  gdk_gl_context_set_required_version(m_gl, 3, 2);
  gdk_gl_context_realize(m_gl, &error);
  if(error) {
    const backend_error ex { error->message };
    g_clear_error(&error);
    g_clear_object(&m_gl);
    throw ex;
  }

  MakeCurrent cur { m_gl };

  glGenTextures(1, &m_tex);
  resizeTextures(m_window->viewport()->Size); // binds to the texture and sets its size

  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  setup();

  // prevent invalidation (= displaying garbage) when moving another window over
  if(!m_window->isDocked())
    gdk_window_freeze_updates(osWindow);
}

GDKOpenGL::~GDKOpenGL()
{
  {
    MakeCurrent cur { m_gl };

    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_tex);

    teardown();
  }

  // current GL context must be cleared before calling unref to avoid this bug:
  // https://gitlab.gnome.org/GNOME/gtk/-/issues/2562
  g_object_unref(m_gl);
}

void GDKOpenGL::initSoftwareBlit()
{
  m_pixels.reset(LICE_CreateBitmap(0, 0, 0));

  static std::weak_ptr<GdkWindow> g_offscreen;

  if(g_offscreen.expired()) {
    GdkWindowAttr attr {};
    attr.window_type = GDK_WINDOW_TOPLEVEL;
    GdkWindow *window { gdk_window_new(nullptr, &attr, 0) };
    std::shared_ptr<GdkWindow> offscreen { window, g_object_unref };
    g_offscreen = m_offscreen = offscreen;
  }
  else
    m_offscreen = g_offscreen.lock();
}

void GDKOpenGL::setSize(const ImVec2 size)
{
  MakeCurrent cur { m_gl };
  resizeTextures(size);
}

void GDKOpenGL::resizeTextures(const ImVec2 size)
{
  // RECT rect;
  // GetClientRect(m_window->nativeHandle(), &rect);
  // const int width  { rect.right - rect.left },
  //           height { rect.bottom - rect.top };
  printf("%dx%d\n", (int)size.x, (int)size.y);
  // printf("%dx%d\n", width, height);

  glBindTexture(GL_TEXTURE_2D, m_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y,
    0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

  if(m_pixels) {
    LICE__resize(m_pixels.get(), size.x, size.y);
    glPixelStorei(GL_PACK_ROW_LENGTH, LICE__GetRowSpan(m_pixels.get()));
  }
}

void GDKOpenGL::render(void *userData)
{
  if(userData && m_pixels)
    return softwareBlit();

  MakeCurrent cur { m_gl };

  // FIXME: Currently we use SWELL's DPI scale which is fixed & app-wide.
  // If this changes, we'll want to only upload textures for our own DPI
  // since we're not sharing them with other windows.
  const bool useSoftwareBlit { m_window->isDocked() };
  OpenGLRenderer::updateTextures();
  OpenGLRenderer::render(useSoftwareBlit);

  if(useSoftwareBlit) {
    // REAPER is also drawing to the same GdkWindow so we must share it.
    // Switch to slower render path, copying pixels into a LICE bitmap.
    glReadPixels(0, 0,
      LICE__GetWidth(m_pixels.get()), LICE__GetHeight(m_pixels.get()),
      GL_BGRA, GL_UNSIGNED_BYTE, LICE__GetBits(m_pixels.get()));
    InvalidateRect(m_window->nativeHandle(), nullptr, false); // post a WM_PAINT
    return;
  }

  GdkWindow *window { static_cast<GDKWindow *>(m_window)->getOSWindow() };
  const ImDrawData *drawData { m_window->viewport()->DrawData };
  const cairo_region_t *region { gdk_window_get_clip_region(window) };
  GdkDrawingContext *drawContext { gdk_window_begin_draw_frame(window, region) };
  cairo_t *cairoContext { gdk_drawing_context_get_cairo_context(drawContext) };
  gdk_cairo_draw_from_gl(cairoContext, window,
    m_tex, GL_TEXTURE, 1, 0, 0,
    drawData->DisplaySize.x * drawData->FramebufferScale.x,
    drawData->DisplaySize.y * drawData->FramebufferScale.y);
  gdk_window_end_draw_frame(window, drawContext);

  // required for making the window visible on GNOME
  gdk_window_thaw_updates(window); // schedules an update
  gdk_window_freeze_updates(window);
}

void GDKOpenGL::swapBuffers(void *)
{
}

void GDKOpenGL::softwareBlit()
{
  PAINTSTRUCT ps;
  if(!BeginPaint(m_window->nativeHandle(), &ps))
    return;

  const int width  { LICE__GetWidth(m_pixels.get())  },
            height { LICE__GetHeight(m_pixels.get()) };

  StretchBltFromMem(ps.hdc, 0, 0, width, height, LICE__GetBits(m_pixels.get()),
    width, height, LICE__GetRowSpan(m_pixels.get()));

  EndPaint(m_window->nativeHandle(), &ps);
}

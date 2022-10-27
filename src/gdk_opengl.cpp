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

struct LICEDeleter {
  void operator()(LICE_IBitmap *bm) { LICE__Destroy(bm); }
};

class GDKOpenGL : public OpenGLRenderer {
public:
  GDKOpenGL(RendererFactory *, GDKWindow *);
  ~GDKOpenGL();

  void uploadFontTex(ImFontAtlas *) override;
  void render(ImGuiViewport *) override;
  void peekMessage(const unsigned int msg) override;

private:
  void initSoftwareBlit();
  void resizeTextures();
  void softwareBlit();

  GDKWindow *m_viewport;
  GdkGLContext *m_gl;
  unsigned int m_tex, m_fbo;

  // for docking
  std::unique_ptr<LICE_IBitmap, LICEDeleter> m_pixels;
  std::shared_ptr<GdkWindow> m_offscreen;
};

std::unique_ptr<Renderer> RendererFactory::create(Window *window)
{
  return std::make_unique<GDKOpenGL>(this, static_cast<GDKWindow *>(window));
}

// GdkGLContext cannot share ressources: they're already shared with the
// window's paint context (which itself isn't shared with anything).
GDKOpenGL::GDKOpenGL(RendererFactory *factory, GDKWindow *viewport)
  : OpenGLRenderer(factory, false), m_viewport { viewport }
{
  GdkWindow *window;

  if(viewport->isDocked()) {
    initSoftwareBlit();
    window = m_offscreen.get();
  }
  else
    window = viewport->getOSWindow();

  if(!window || static_cast<void *>(window) == viewport->nativeHandle())
    throw backend_error { "headless SWELL is not supported" };

  GError *error {};
  m_gl = gdk_window_create_gl_context(window, &error);
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
  resizeTextures(); // binds to the texture and sets its size

  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  setup();

  // prevent invalidation (= displaying garbage) when moving another window over
  if(!viewport->isDocked())
    gdk_window_freeze_updates(window);
}

GDKOpenGL::~GDKOpenGL()
{
  gdk_gl_context_make_current(m_gl);

  glDeleteFramebuffers(1, &m_fbo);
  glDeleteTextures(1, &m_tex);

  teardown();

  // current GL context must be cleared before calling unref to avoid this bug:
  // https://gitlab.gnome.org/GNOME/gtk/-/issues/2562
  gdk_gl_context_clear_current();
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

void GDKOpenGL::resizeTextures()
{
  RECT rect;
  GetClientRect(m_viewport->nativeHandle(), &rect);
  const int width  { rect.right - rect.left },
            height { rect.bottom - rect.top };

  glBindTexture(GL_TEXTURE_2D, m_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
    0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

  if(m_pixels) {
    LICE__resize(m_pixels.get(), width, height);
    glPixelStorei(GL_PACK_ROW_LENGTH, LICE__GetRowSpan(m_pixels.get()));
  }
}

void GDKOpenGL::uploadFontTex(ImFontAtlas *atlas)
{
  MakeCurrent cur { m_gl };
  OpenGLRenderer::uploadFontTex(atlas);
}

void GDKOpenGL::render(ImGuiViewport *viewport)
{
  MakeCurrent cur { m_gl };

  const bool useSoftwareBlit { m_viewport->isDocked() };
  OpenGLRenderer::render(viewport, useSoftwareBlit);

  if(useSoftwareBlit) {
    // REAPER is also drawing to the same GdkWindow so we must share it.
    // Switch to slower render path, copying pixels into a LICE bitmap.
    glReadPixels(0, 0,
      LICE__GetWidth(m_pixels.get()), LICE__GetHeight(m_pixels.get()),
      GL_BGRA, GL_UNSIGNED_BYTE, LICE__GetBits(m_pixels.get()));
    InvalidateRect(m_viewport->nativeHandle(), nullptr, false);
    return;
  }

  GdkWindow *window { m_viewport->getOSWindow() };
  const ImDrawData *drawData { m_viewport->viewport()->DrawData };
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

void GDKOpenGL::peekMessage(const unsigned int msg)
{
  switch(msg) {
  case WM_SIZE: {
    MakeCurrent cur { m_gl };
    resizeTextures();
    break;
  }
  case WM_PAINT:
    if(m_pixels)
      softwareBlit();
    break;
  }
}

void GDKOpenGL::softwareBlit()
{
  PAINTSTRUCT ps;
  if(!BeginPaint(m_viewport->nativeHandle(), &ps))
    return;

  const int width  { LICE__GetWidth(m_pixels.get())  },
            height { LICE__GetHeight(m_pixels.get()) };

  StretchBltFromMem(ps.hdc, 0, 0, width, height, LICE__GetBits(m_pixels.get()),
    width, height, LICE__GetRowSpan(m_pixels.get()));

  EndPaint(m_viewport->nativeHandle(), &ps);
}

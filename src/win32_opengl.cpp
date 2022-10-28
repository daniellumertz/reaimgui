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
#include "window.hpp"

#include <GL/gl3w.h>
#include <GL/wglext.h>

class Win32OpenGL : public OpenGLRenderer {
public:
  Win32OpenGL(RendererFactory *, HWND);
  ~Win32OpenGL();

  void render(ImGuiViewport *, const TextureManager *) override;

private:
  void setPixelFormat();
  void createContext();

  HWND m_hwnd;
  HDC m_dc;
  HGLRC m_gl;
};

class MakeCurrent {
public:
  MakeCurrent(HDC dc, HGLRC gl)
    : m_gl { gl }
  {
    wglMakeCurrent(dc, m_gl);
  }

  ~MakeCurrent()
  {
    wglMakeCurrent(nullptr, nullptr);
  }

private:
  HGLRC m_gl;
};

struct GLDeleter {
  void operator()(HGLRC gl) { wglDeleteContext(gl); }
};

std::unique_ptr<Renderer> RendererFactory::create(Window *viewport)
{
  return std::make_unique<Win32OpenGL>(this, viewport->nativeHandle());
}

Win32OpenGL::Win32OpenGL(RendererFactory *factory, HWND hwnd)
  : OpenGLRenderer(factory), m_hwnd { hwnd }, m_dc { GetDC(hwnd) }
{
  setPixelFormat();

  if(m_shared->m_platform) {
    using GL = std::remove_pointer_t<HGLRC>;
    m_gl = std::static_pointer_cast<GL>(m_shared->m_platform).get();
  }
  else {
    createContext();
    m_shared->m_platform = { m_gl, GLDeleter{} };
  }

  MakeCurrent cur { m_dc, m_gl };
  setup();
}

Win32OpenGL::~Win32OpenGL()
{
  MakeCurrent cur { m_dc, m_gl };
  teardown();

  ReleaseDC(m_hwnd, m_dc);
}

void Win32OpenGL::setPixelFormat()
{
  PIXELFORMATDESCRIPTOR pfd {};
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cAlphaBits = pfd.cBlueBits = pfd.cGreenBits = pfd.cRedBits = 8;
  pfd.cColorBits = pfd.cRedBits + pfd.cGreenBits + pfd.cBlueBits + pfd.cAlphaBits;

  if(!SetPixelFormat(m_dc, ChoosePixelFormat(m_dc, &pfd), &pfd)) {
    ReleaseDC(m_hwnd, m_dc);
    throw backend_error { "failed to set a suitable pixel format" };
  }
}

void Win32OpenGL::createContext()
{
  HGLRC dummyGl { wglCreateContext(m_dc) }; // creates a legacy (< 2.1) context
  wglMakeCurrent(m_dc, m_gl = dummyGl);

  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB
    { reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>
      (wglGetProcAddress("wglCreateContextAttribsARB")) };

  if(wglCreateContextAttribsARB) {
    static int minor { 2 };
    do {
      // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
      const int attrs[] {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, minor,
        0
      };

      if(HGLRC coreGl { wglCreateContextAttribsARB(m_dc, nullptr, attrs) }) {
        wglMakeCurrent(m_dc, m_gl = coreGl);
        wglDeleteContext(dummyGl);
        break;
      }
    } while(--minor >= 1);
  }

  if(gl3wInit()) {
    wglDeleteContext(m_gl);
    ReleaseDC(m_hwnd, m_dc);
    throw backend_error { "failed to initialize OpenGL 3.1+ context" };
  }
}

void Win32OpenGL::render(ImGuiViewport *viewport, const TextureManager *manager)
{
  MakeCurrent cur { m_dc, m_gl };
  OpenGLRenderer::updateTextures(manager);
  OpenGLRenderer::render(viewport, false);
  SwapBuffers(m_dc);
}

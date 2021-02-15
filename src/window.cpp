#include "window.hpp"

#include "context.hpp"

#include <reaper_plugin_functions.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <swell/swell.h>
#  include <WDL/wdltypes.h>
#endif

HINSTANCE Window::s_instance;

LRESULT CALLBACK Window::proc(HWND handle, const unsigned int msg,
  const WPARAM wParam, const LPARAM lParam)
{
  Context *ctx {
    reinterpret_cast<Context *>(GetWindowLongPtr(handle, GWLP_USERDATA))
  };

  if(!ctx || !ctx->window())
    return DefWindowProc(handle, msg, wParam, lParam);
  else if(ctx->window()->handleMessage(msg, wParam, lParam))
    return 0;

  switch(msg) {
  case WM_CLOSE:
    ctx->setCloseRequested();
    return 0;
  case WM_DESTROY:
    SetWindowLongPtr(handle, GWLP_USERDATA, 0);
    return 0;
  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL:
#ifndef GET_WHEEL_DELTA_WPARAM
#  define GET_WHEEL_DELTA_WPARAM GET_Y_LPARAM
#endif
    ctx->mouseWheel(msg, GET_WHEEL_DELTA_WPARAM(wParam));
    break;
  case WM_SETCURSOR:
    if(LOWORD(lParam) == HTCLIENT) {
      SetCursor(ctx->cursor()); // sets the cursor when re-entering the window
      return 1;
    }
#ifdef _WIN32
    break; // lets Windows set the cursor over resize handles
#else
    return 1; // tells SWELL to not reset the cursor to IDC_ARROW on mouse events
#endif
#ifndef __APPLE__ // these are handled by InputView, bypassing SWELL
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
    ctx->mouseDown(msg);
    return 0;
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    ctx->mouseUp(msg);
    return 0;
#endif // __APPLE__
  }

  return DefWindowProc(handle, msg, wParam, lParam);
}

#ifndef _WIN32
HWND Window::createSwellDialog(const char *title)
{
  enum SwellDialogResFlags {
    ForceNonChild = 0x400000 | 0x8, // allows not using a resource id
    Resizable = 1,
  };

  const char *res { MAKEINTRESOURCE(ForceNonChild | Resizable) };
  HWND dialog { CreateDialog(s_instance, res, parentHandle(), proc) };
  SetWindowText(dialog, title);
  return dialog;
}
#endif

HWND Window::parentHandle()
{
  return GetMainHwnd();
}

void Window::WindowDeleter::operator()(HWND window)
{
  DestroyWindow(window);
}

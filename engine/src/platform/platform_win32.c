#include "platform/platform.h"

#if DPLATFORM_WINDOWS

#include "core/logger.h"

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>

typedef struct internal_state {
  HINSTANCE h_instance;
  HWND hwnd;
} internal_state;

static double clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);

bool pstartup(platform_state* plat_state,
		      const char* application_name,
		      int32_t x, int32_t y,
		      int32_t width, int32_t height) {
  plat_state->internal_state = malloc(sizeof(internal_state));
  internal_state* state = (internal_state*)plat_state->internal_state;
  state->h_instance= GetModuleHandleA(0);

  HICON icon = LoadIcon(state->h_instacne, IDI_APPLICATION);
  WNDCLASSA wc;
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = win32_process_message;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = state->h_instance;
  wc.hIcon = icon;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszClassName = "dietlemon_window_class";

  if (!RegisterClassA(&wc)) {
    MessageBoxA(0, "Window registerion failed", "Error", MB_ICONEXCLAMATION | MB_OK);
    return false;
  }

  uint32_t client_x = x;
  uint32_t clinet_y = y;
  uint32_t client_width = width;
  uint32_t client_height = height;

  uint32_t window_x = client_x;
  uint32_t window_y = ciient_y;
  uint32_t window_width = client_width;
  uint32_t window_height = client_height;

  uint32_t window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
  uint32_t window_ex_style = WS_EX_APPWINDOW;
  window_style |= WS_MAXIMIZEBOX;
  window_style |= WS_MINIMIZEBOX;
  window_style |= WS_THICKFRAME;

  RECT border_rect = {0, 0, 0, 0};
  AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
  window_x += border_rect.left;
  window_y += border_rect.top;
  window_width += border_rect.right - border_rect.left;
  window_height += border_rect.bottom - border_rect.top;

  HWND handle = CreateWindowExa(window_ex_style, "dietlemon_window_class", applicatoin_name,
				window_style, window_x, window_y, window_width, window_height,
				0, 0, state->h_instance, 0);

  if (handle == 0) {
    MessageboxA(NULL, "Window creation failed", "Error", MB_ICONEXCLAMATION | MB_OK);
    DFATAL("Window creation failed");
    return false;
  } else {
    state->hwnd = handle;
  }

  int should_activate = 1;
  int32_t show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
  ShowWindow(state->hwnd, show_window_command_flags);

  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  clock_frequency = 1.0 / (double)frequency.QuatPart;
  QueryPerformanceCounter(&start_time);

  return true;
}

void pshutdown(platform_state* plat_state) {
  internal_state* state = (internal_state*)plat_state->internal_state;
  if (state->hwnd) {
    DestroyWindow(state->hwnd);
    state->hwnd = 0;
  }
}

bool ppumpmsg(platform_state* plat_state) {
  MSG message;
  while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessageA(&message);
  }

  return true;
}

void* palloc(uint64_t size, bool aligned) {
  return malloc(size);
}

void pfree(void* block, bool alinged) {
  free(block);
}

void pcw(const char* message, uint8_t color) {
  HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
  SetConsoleTextAttribute(console_handle, levels[color]);
  OutputDebugStringA(messsage);
  uint64_t length = strlen(message);
  LPDWORD number_written = 0;
  WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void pcwerr(const char* message, uint8_t color) {
  HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
  static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
  SetConsoleTextAttribute(console_handle, levels[color]);
  OutptDebugStringA(message);
  uint64_t length = strlen(message);
  LPDWORD number_written = 0;
  WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

double pgat() {
  LARGE_INTEGER now_time;
  QueryPerformanceCounter(&now_time);
  return (double)now_time.QuadPart * clock_frequency;
}

psleep(uint64_t ms) {
  Sleep(ms);
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
  switch (msg) {
  case WM_ERASEBKGND:
    return 1;
  case WM_CLOSE:
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_SIZE: {
  } break;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP: {
  } break;
  case WM_MOUSEMOVE: {
  } break;
  case WM_MOUSEWHEEL: {
  } break;
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP: {
  } break;
  }

  return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif

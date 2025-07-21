#include <Windows.h>
#include <iostream>
#include <vector>
#include <winuser.h>

int posX = 0;
int posY = 0;
float horizontalV = 0.0f;
float verticalV = 0.0f;
float gravity = 0.8f;
float friction = 0.98f;
float bounceFactor = 0.5f;
bool isDragging = false; // If user dragged the window

struct Position {
  int x, y;
  DWORD timestamp;
};

std::vector<Position> recentPositions;
const int MAX_POSITION_HISTORY = 10;
const int MIN_DISTANCE = 50;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
#define IDT_TIMER1 1
  switch (uMsg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_CREATE:
    SetTimer(hwnd, IDT_TIMER1, 16, NULL); // ~60 FPS
    return 0;
  case WM_ENTERSIZEMOVE:
    isDragging = true;

    horizontalV = 0;
    verticalV = 0;
    return 0;
  case WM_EXITSIZEMOVE: // Stopped dragging
    isDragging = false;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    posX = rect.left;
    posY = rect.top;
    return 0;
  case WM_MOVING: {

    RECT *pRect = (RECT *)lParam;
    DWORD currentTime = GetTickCount();
    Position pos = {pRect->left, pRect->top, currentTime};

    if (recentPositions.size() != 0) {
      int dx = pos.x - recentPositions.back().x;
      int dy = pos.y - recentPositions.back().y;
      if (dx * dx + dy * dy < MIN_DISTANCE * MIN_DISTANCE)
        return 0; // Skip insignificant jitter
    }

    std::cout << "X:" << pos.x << ", Y:" << pos.y << std::endl;
    recentPositions.push_back(pos);
    if (recentPositions.size() > MAX_POSITION_HISTORY) {
      recentPositions.erase(recentPositions.begin());
    }

    // Calculate velocities
    if (recentPositions.size() >= 2) {
      Position &first = recentPositions.front();
      Position &last = recentPositions.back();

      float deltaTime = (last.timestamp - first.timestamp) / 1000.0f;
      if (deltaTime < 0.05f)
        deltaTime = 0.05f;
      if (deltaTime > 0.0f) {
        horizontalV = (last.x - first.x) / deltaTime;
        verticalV = (last.y - first.y) / deltaTime;
      }
    }
    break;
  }
  case WM_TIMER:
    if (wParam == IDT_TIMER1 && !isDragging) {
      RECT rect;
      RECT workArea;
      GetWindowRect(hwnd, &rect);
      int windowHeight = rect.bottom - rect.top;
      int windowWidth = rect.right - rect.left;
      SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
      int screenHeight = workArea.bottom - workArea.top;
      int screenWidth = workArea.right - workArea.left;

      verticalV += gravity;

      verticalV *= friction;
      horizontalV *= friction;
      if (std::abs(horizontalV) < 0.01f)
        horizontalV = 0;

      posY += (int)verticalV;
      posX += (int)horizontalV;

      // Vertical bounce
      if (posY <= 0) {
        posY = 0;
        verticalV = -verticalV * bounceFactor;
      } else if (posY + windowHeight >= screenHeight) {
        posY = screenHeight - windowHeight;
        verticalV = -verticalV * bounceFactor;
      }

      // Horizontal bounce
      if (posX <= 0) {
        posX = 0;
        horizontalV = -horizontalV * bounceFactor;
      } else if (posX + windowWidth >= screenWidth) {
        posX = screenWidth - windowWidth;
        horizontalV = -horizontalV * bounceFactor;
      }

      SetWindowPos(hwnd, HWND_TOPMOST, posX, posY, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER);
    }
    return 0;
  }
  return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASSW wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"BouncyWindowClass";

  RegisterClassW(&wc);

  HWND hwnd = CreateWindowExW(
      WS_EX_TOPMOST,                               // Extended style
      L"BouncyWindowClass",                        // Class name
      L"bouncy",                                   // Window title
      WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, // Style
      CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

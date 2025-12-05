/*
mousekeys.cpp

A single-file Windows C++ program that lets you control the system mouse cursor
with the keyboard using smooth, physics-style movement (like the TouHou
Project), but moves the real OS cursor so you can interact with other apps and
the desktop.

Controls
> Arrow keys or H, J, K, and L for movement
> 'Z' for Left Click
> 'X' for Right Click
> Capslock On/Off for Enable/Disable respectively

Features
- Global low-level keyboard hook (WH_KEYBOARD_LL) so you can control the cursor
even when other windows are focused.
- Starts off---capslock must be clicked to activate
- Smooth acceleration, velocity, and friction (configurable).
- Safely swallows the movement keys while control is enabled so they don't also
act in other apps.
- Runs as a simple user-mode app (no admin required normally).

Notes:
- The keyboard hook is per-session; it doesn't require a separate DLL for the
low-level hook.
- Antivirus or system security software sometimes flags programs that install
global hooks.
- Be careful: when the controller is enabled, arrow keys/HJKL will not be
delivered to other apps. Toggle with Capslock to return normal keyboard
behavior.

Build (MSVC): cl /EHsc /O2 mousekeys.cpp user32.lib gdi32.lib

Build (MinGW): g++ -std=c++17 -O2 -mwindows mousekeys.cpp -lgdi32 -luser32 -o
mousekeys.exe
*/

#define WIN32_LEAN_AND_MEAN
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>
#include <windows.h>

// --- Configuration (tweak to match feel) ---
static constexpr float ACCEL_PIX_PER_S2 = 10000.0f; // ammount of acceleration when key held
static constexpr float MAX_SPEED_PIX_PER_S = 700.0f; // top speed in pixels/sec
static constexpr float FRICTION_PER_S = 1000.0f; // amount of friction
static constexpr int UPDATES_PER_SEC = 120; // physics loop frequency

// Keys: movement keys and click keys
static constexpr int LEFT_CLICK_KEY = 'Z';
static constexpr int RIGHT_CLICK_KEY = 'X';

// Key state storage (we'll track both arrow keys and WASD)
std::atomic<bool> key_up(false), key_down(false), key_left(false), key_right(false);
std::atomic<bool> key_k(false), key_h(false), key_j(false), key_l(false);
std::atomic<bool> leftClickPressed(false), rightClickPressed(false);
std::atomic<bool> shiftPressed(false);

// Add these globals for tracking previous mouse-key state (for drag/cleanup)
std::atomic<bool> g_prevLeft(false), g_prevRight(false);

// Controller state
std::atomic<bool> enabled(false);
std::atomic<bool> running(true);

// Low-level keyboard hook handle
HHOOK g_hHook = nullptr;

// Utility: send a mouse click (left or right). Single click: down then up.
void sendMouseClick(bool left) {
   INPUT inputs[2] = {};
   ZeroMemory(inputs, sizeof(inputs));
   inputs[0].type = INPUT_MOUSE;
   inputs[1].type = INPUT_MOUSE;
   if (left) {
      inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
      inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
   } else {
      inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
      inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
   }
   SendInput(2, inputs, sizeof(INPUT));
}

void sendMouseDown(bool left) {
   INPUT input = {};
   input.type = INPUT_MOUSE;
   input.mi.dwFlags = left ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
   SendInput(1, &input, sizeof(INPUT));
}

void sendMouseUp(bool left) {
   INPUT input = {};
   input.type = INPUT_MOUSE;
   input.mi.dwFlags = left ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
   SendInput(1, &input, sizeof(INPUT));
}

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
   if (nCode < 0) {
      return CallNextHookEx(g_hHook, nCode, wParam, lParam);
   }
   
   KBDLLHOOKSTRUCT *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
   bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
   bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
   
   // Toggle on key down of Right Shift or Caps Lock
   if (isDown && (kb->vkCode == VK_RSHIFT || kb->vkCode == VK_CAPITAL)) {
      bool now = enabled.load();
      enabled.store(!now);
      
      return 1;
   }
   
   // Map movement keys when controller is enabled
   if (enabled.load()) {
      // Update our internal key state and swallow movement keys and click keys
      switch (kb->vkCode) {
         case VK_UP:
            if (isDown) key_up.store(true);
            if (isUp) key_up.store(false);
         return 1; // swallow when enabled
         case VK_DOWN:
            if (isDown) key_down.store(true);
            if (isUp) key_down.store(false);
         return 1;
         case VK_LEFT:
            if (isDown) key_left.store(true);
            if (isUp) key_left.store(false);
         return 1;
         case VK_RIGHT:
            if (isDown) key_right.store(true);
            if (isUp) key_right.store(false);
         return 1;
         case 'K':
            if (isDown) key_k.store(true);
            if (isUp) key_k.store(false);
            return 1;
         case 'J':
            if (isDown) key_j.store(true);
            if (isUp) key_j.store(false);
         return 1;
         case 'H':
            if (isDown) key_h.store(true);
            if (isUp) key_h.store(false);
         return 1;
         case 'L':
            if (isDown) key_l.store(true);
            if (isUp) key_l.store(false);
         return 1;
         case LEFT_CLICK_KEY:
            if (isDown) leftClickPressed.store(true);
            if (isUp) leftClickPressed.store(false);
         return 1;
         case RIGHT_CLICK_KEY:
            if (isDown) rightClickPressed.store(true);
            if (isUp) rightClickPressed.store(false);
         return 1;
         case VK_LSHIFT:
            if (isDown) shiftPressed.store(true);
            if (isUp) shiftPressed.store(false);
         return 1; // allow the key to pass through to other apps
         default:
         break;
      }
   } else {
      key_up.store(false);
      key_down.store(false);
      key_left.store(false);
      key_right.store(false);
      key_k.store(false);
      key_j.store(false);
      key_h.store(false);
      key_l.store(false);
      shiftPressed.store(false);
   }
   
   // If not enabled, or other keys, pass through
   return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// Physics & cursor movement loop that runs in its own thread
void physicsLoop() {
   // Get initial cursor position
   POINT p;
   if (!GetCursorPos(&p)) {
      p.x = GetSystemMetrics(SM_CXSCREEN) / 2;
      p.y = GetSystemMetrics(SM_CYSCREEN) / 2;
   }
   
   // Vars initialize/reinitialize
   double px = (double)p.x;
   double py = (double)p.y;
   // double vx = 0.0;
   // double vy = 0.0;
   float dx = 0.0f;
   float dy = 0.0f;
   
   using clock = std::chrono::high_resolution_clock;
   auto last = clock::now();
   const double targetDt = 1.0 / UPDATES_PER_SEC;
   
   while (running.load()) {
      auto now = clock::now();
      std::chrono::duration<double> elapsed = now - last;
      double dt = elapsed.count();

      if (dt < 1e-9) dt = targetDt;
      
      // Clamp dt to avoid huge jumps
      if (dt > 0.05) dt = 0.05;
      last = now;
      
      // If control enabled
      if (enabled.load()) {
         // Reinitialize vectors so cursor stops moving when no dir is pressed
         dx = 0.0f;
         dy = 0.0f;
         
         // Gather direction from key states
         if (key_up.load() || key_k.load())    dy -= 1.0f;
         if (key_down.load() || key_j.load())  dy += 1.0f;
         if (key_left.load() || key_h.load())  dx -= 1.0f;
         if (key_right.load() || key_l.load()) dx += 1.0f;
         
         // Normalize so diagonal isn't faster
         float speed = std::hypot(dx, dy);
         if (speed > 0.0f) {
            dx /= speed;
            dy /= speed;
         }
         
         // // Apply acceleration
         // vx += dx * ACCEL_PIX_PER_S2 * dt;
         // vy += dy * ACCEL_PIX_PER_S2 * dt;
         
         // // Apply exponential friction
         // float decay = std::expf(-FRICTION_PER_S * (float)dt);
         // vx *= decay;
         // vy *= decay;
         
         // // Clamp speed
         // double speed = std::hypot(vx, vy);
         // if (speed > MAX_SPEED_PIX_PER_S) {
         //   double s = MAX_SPEED_PIX_PER_S / speed;
         //   vx *= s;
         //   vy *= s;
         // }
         
         // // Integrate
         // px += vx * dt;
         // py += vy * dt;
         
         // Vanilla speed calcs w/ speed modifier
         float speedMult = shiftPressed.load() ? 0.5f : 1.0f;
         float move = MAX_SPEED_PIX_PER_S * speedMult * (float)dt;
         px += dx * move;
         py += dy * move;
         
         // Clamp to screen bounds
         int screenW = GetSystemMetrics(SM_CXSCREEN);
         int screenH = GetSystemMetrics(SM_CYSCREEN);
         if (px < 0.0) {
            px = 0.0;
            //vx = 0.0;
         }
         if (py < 0.0) {
            py = 0.0;
            //vy = 0.0;
         }
         if (px > screenW - 1) {
            px = screenW - 1;
            //vx = 0.0;
         }
         if (py > screenH - 1) {
            py = screenH - 1;
            //vy = 0.0;
         }
         
         // Move cursor
         SetCursorPos((int)std::lround(px), (int)std::lround(py));
         
         // Look for key clicks and enable dragging
         bool prevLeft = g_prevLeft.load();
         bool prevRight = g_prevRight.load();
         bool curLeft = leftClickPressed.load();
         bool curRight = rightClickPressed.load();
         if (curLeft && !prevLeft) {
            sendMouseDown(true); // start a drag (mouse button down)
         }
         if (!curLeft && prevLeft) {
            sendMouseUp(true); // end drag (mouse button up)
         }
         if (curRight && !prevRight) {
            sendMouseDown(false);
         }
         if (!curRight && prevRight) {
            sendMouseUp(false);
         }
         
         g_prevLeft.store(curLeft);
         g_prevRight.store(curRight);
      } else {
         // // If disabled, slowly zero velocity (so it doesn't fling when re-enabled)
         // vx *= 0.6;
         // vy *= 0.6;
         
         // If disabled (vanilla), set velocity to zero << *for some reason this isn't necessary*
         // dx *= 0.0;
         // dy *= 0.0;
         
         // Keeps px/py synced with current cursor location (when user moves with mouse)
         POINT curp;
         if (GetCursorPos(&curp)) {
            px = (double)curp.x;
            py = (double)curp.y;
         }
      }
      
      // Sleep to approximate target update rate (use high-res sleep)
      std::this_thread::sleep_for(std::chrono::duration<double>(targetDt));
   }
}

// Minimal hidden window to keep message loop alive (hooks require a message loop in the thread)
HWND createMessageWindow(HINSTANCE hInstance) {
   const wchar_t CLASSNAME[] = L"MouseKeysHiddenWindow";
   WNDCLASSEXW wcx = {};
   wcx.cbSize = sizeof(wcx);
   wcx.lpfnWndProc = DefWindowProc;
   wcx.hInstance = hInstance;
   wcx.lpszClassName = CLASSNAME;
   RegisterClassExW(&wcx);
   HWND hwnd = CreateWindowExW(0, CLASSNAME, L"MouseKeysHidden", 0, 0, 0, 0, 0,
      HWND_MESSAGE, NULL, hInstance, NULL);
      return hwnd;
}
   
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
   //// Optional: allocate console for debug output
   // AllocConsole();
   // FILE *f;
   // freopen_s(&f, "CONOUT$", "w", stdout);
   // std::cout << "MouseKeys: starting. Press CAPS LOCK to toggle, Ctrl+Shift+Q
   // to quit.\n"; std::cout << "When enabled: Arrow keys or WASD move the
   // cursor. Z = left click, X = right click.\n"; std::cout << std::endl;
   
   // Create message-only window (so hook thread has a message pump)
   HWND hwnd = createMessageWindow(hInstance);
   
   // Install low-level keyboard hook on this thread (global for the session)
   g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
   
   // On keyboard hook install failure
   if (!g_hHook) {
      MessageBoxW(NULL, L"Failed to install keyboard hook. Exiting.",L"mousekeys", MB_ICONERROR);
      return 1;
   }
      
   // Start physics thread
   std::thread phys(physicsLoop);
   
   // Simple message loop to keep process alive and handle hook/event dispatch
   MSG msg;
   while (running.load() && GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
   
   // Cleanup
   running.store(false);
   if (g_hHook) {
      UnhookWindowsHookEx(g_hHook);
      g_hHook = nullptr;
   }

   // Wait for physics thread to finish
   if (phys.joinable()) phys.join();
   
   //// Free console optionally
   // FreeConsole();

   // After physics and hook cleanup (before return)
   if (g_prevLeft.load()) sendMouseUp(true);
   if (g_prevRight.load()) sendMouseUp(false);

   return 0;
}
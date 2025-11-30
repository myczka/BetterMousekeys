# <i>Better Mouse Keys</i>

### What It Is
- A small Windows program that moves the actual system cursor using keyboard keys with smooth physics (acceleration + inertia + friction).
- Works globally (uses a low-level keyboard hook), so you can control other apps and the desktop without a physical mouse.
- Toggle the controller on/off with Capslock.
- Click keys: Z => left-click, X => right-click (while controller is enabled).
- Quit the app with Ctrl+Shift+Q.

How it behaves differently from built-in "Mouse Keys"
- Windows "Mouse Keys" snaps the cursor in grid-like steps. This program simulates velocity and acceleration so movement is smooth and continuous (like a game joystick), which can be easier for fine control or for a different feel.

Security & safety notes
- Global hooks are powerful. Some security products may flag this as suspicious.
- The program swallows movement keys while enabled (so arrow keys/WASD won't be delivered to other apps while you're controlling the cursor). Toggle off to restore normal keyboard behavior.

Build instructions
- You need a C++ compiler for Windows:
  - MSVC (Visual Studio) or MinGW (g++).
- Example MSVC build:
    cl /EHsc /O2 mousekeys.cpp user32.lib gdi32.lib

- Example MinGW build:
    g++ -std=c++17 -O2 -mwindows mousekeys.cpp -lgdi32 -luser32 -o mousekeys.exe

Run
- Launch the program (double-click the exe or run from a console).
- The app creates a hidden message window and installs the keyboard hook.
- Press F12 to enable the smooth mouse control. Use arrow keys or WASD to move.
- Press Z to left-click, X to right-click (registered on key-down).
- Press F12 again to disable and return keys to normal.
- Press Ctrl+Shift+Q to quit the program.

Tuning the feel
- Edit the top of mousekeys.cpp to change:
  - ACCEL_PIX_PER_S2 (acceleration)
  - MAX_SPEED_PIX_PER_S (top speed)
  - FRICTION_PER_S (damping)
  - UPDATES_PER_SEC (physics update frequency)

Potential improvements
- Add an on-screen HUD or tray icon to show enabled/disabled.
- Provide a configuration UI to set sensitivity and key bindings.
- Persist settings or provide a system tray menu.

If you want, I can:
- Produce a version that maps additional keys, adds click-and-drag support, or provides a system tray indicator with an enable/disable toggle.
- Add a small GUI for adjusting sensitivity and bindings.
```
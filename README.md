# <i>Better Mouse Keys</i>

### What It Is
Better Mouse Keys is a small C++ program that allows the user to control the actual system cursor using keyboard keys. It works globally (uses a low-level keyboard hook), so you can control other apps and the desktop without a physical mouse. This program aims to provide proper keyboard-mouse control to the user that Windows' Mouse Keys fail to deliver. Instead of a snap-to-grid movement system, this program implements mouse control emulation to allow seamless and intuitive movement similar to that of games like <i>UNDERTALE</i> and the <i>Touhou Project</i>.

### Controls
- Toggle the controller on/off with Capslock.
- Arrow keys/hjkl for directional control
- 'z' for left-click
- 'x' for right-click
- Hold <i>'shift'</i> to reduce speed

Security & safety notes
- Global hooks are powerful. Some security products may flag this as suspicious.
- The program swallows all keys that are listed in the controls while enabled (so arrow keys, hjkl, and Lshift won't be delivered to other apps while you're controlling the cursor). You must toggle off to restore normal keyboard behavior.

Build instructions
- You need a C++ compiler for Windows: MSVC (Visual Studio) or MinGW (g++)

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

# Project instructions for agents

Zephys is a clean, lightweight terminal emulator for Linux written in C99 using GTK3 and VTE.

## Build and install

Dependencies on Debian, Ubuntu, and Linux Mint:
```bash
sudo apt install -y build-essential libgtk-3-dev libvte-2.91-dev
```

Common commands:
- Build: `make`
- User install: `./install.sh` or `make install-user`
- System install: `sudo ./install.sh` or `sudo make install`
- Clean: `make clean`

Always run `make` after editing C files to ensure the build passes without warnings before finishing tasks.

## Code structure

- `src/main.c`: program entry, command line argument parsing, and top-level GTK window setup.
- `src/terminal.c`, `src/terminal.h`: tab creation, VTE terminal spawning, working directory tracking, and tab title updates.
- `src/shortcuts.c`, `src/shortcuts.h`: keyboard bindings and right-click context menu actions.
- `src/config.c`, `src/config.h`: configuration file reader and writer (`~/.config/zephys/config.ini`).
- `src/preferences.c`, `src/preferences.h`: GTK preferences dialog for font, opacity, and themes.
- `src/themes.c`, `src/themes.h`, `src/monokai.h`: terminal color palettes and tab CSS styling.
- `assets/`: screenshots and media assets used in documentation.
- `integrations/`: helper and action files for file managers (Thunar, Nemo, Dolphin, Nautilus, PCManFM).

## Guidelines

- Use standard C99 and GLib / GTK3 conventions.
- Keep the terminal lightweight. Avoid adding heavy dependencies or background polling loops.
- Handle file paths safely with GLib utilities such as `g_canonicalize_filename` and `g_file_test`.
- If modifying CLI options or desktop integration, update `install.sh` and `README.md` to keep them consistent.

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=gnu99
PKG_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 vte-2.91 2>/dev/null)
PKG_LIBS := $(shell pkg-config --libs gtk+-3.0 vte-2.91 2>/dev/null)

PREFIX ?= /usr/local
BIN = zephys
SRCS = src/main.c src/terminal.c src/shortcuts.c src/config.c src/preferences.c src/themes.c
OBJS = $(SRCS:.c=.o)

all: check-deps $(BIN)

debug: CFLAGS = -Wall -Wextra -g -O0 -fsanitize=address,undefined -std=gnu99
debug: PKG_LIBS += -fsanitize=address,undefined
debug: clean $(BIN)

check-deps:
	@pkg-config --exists gtk+-3.0 vte-2.91 || ( \
		echo "Error: missing gtk+-3.0 or vte-2.91 development libraries."; \
		echo "Install with: sudo apt install -y libgtk-3-dev libvte-2.91-dev"; \
		exit 1; \
	)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $(BIN) $(PKG_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)

install: $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 zephys.desktop $(DESTDIR)$(PREFIX)/share/applications/zephys.desktop
	install -d $(DESTDIR)$(PREFIX)/share/xfce4/helpers
	install -m 644 integrations/xfce4-helper-zephys.desktop $(DESTDIR)$(PREFIX)/share/xfce4/helpers/zephys.desktop
	install -d $(DESTDIR)$(PREFIX)/share/nemo/actions
	install -m 644 integrations/nemo-action-zephys.nemo_action $(DESTDIR)$(PREFIX)/share/nemo/actions/zephys.nemo_action
	install -d $(DESTDIR)$(PREFIX)/share/kio/servicemenus
	install -m 644 integrations/dolphin-servicemenu-zephys.desktop $(DESTDIR)$(PREFIX)/share/kio/servicemenus/zephys.desktop
	install -d $(DESTDIR)$(PREFIX)/share/file-manager/actions
	install -m 644 integrations/pcmanfm-action-zephys.desktop $(DESTDIR)$(PREFIX)/share/file-manager/actions/zephys.desktop

install-user: $(BIN)
	install -d $(HOME)/.local/bin
	install -m 755 $(BIN) $(HOME)/.local/bin/$(BIN)
	install -d $(HOME)/.local/share/applications
	install -m 644 zephys.desktop $(HOME)/.local/share/applications/zephys.desktop
	install -d $(HOME)/.local/share/xfce4/helpers
	install -m 644 integrations/xfce4-helper-zephys.desktop $(HOME)/.local/share/xfce4/helpers/zephys.desktop
	install -d $(HOME)/.local/share/nemo/actions
	install -m 644 integrations/nemo-action-zephys.nemo_action $(HOME)/.local/share/nemo/actions/zephys.nemo_action
	install -d $(HOME)/.local/share/kio/servicemenus
	install -m 644 integrations/dolphin-servicemenu-zephys.desktop $(HOME)/.local/share/kio/servicemenus/zephys.desktop
	install -d $(HOME)/.local/share/nautilus/scripts
	install -m 755 integrations/nautilus-script-zephys "$(HOME)/.local/share/nautilus/scripts/Open in Zephys"
	install -d $(HOME)/.local/share/file-manager/actions
	install -m 644 integrations/pcmanfm-action-zephys.desktop $(HOME)/.local/share/file-manager/actions/zephys.desktop
	@mkdir -p $(HOME)/.config/xfce4
	@if [ -f $(HOME)/.config/xfce4/helpers.rc ]; then \
		grep -q '^TerminalEmulator=' $(HOME)/.config/xfce4/helpers.rc && \
		sed -i 's/^TerminalEmulator=.*/TerminalEmulator=zephys/' $(HOME)/.config/xfce4/helpers.rc || \
		echo 'TerminalEmulator=zephys' >> $(HOME)/.config/xfce4/helpers.rc; \
	else \
		echo 'TerminalEmulator=zephys' > $(HOME)/.config/xfce4/helpers.rc; \
	fi
	@which update-desktop-database >/dev/null 2>&1 && update-desktop-database $(HOME)/.local/share/applications || true

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
	rm -f $(DESTDIR)$(PREFIX)/share/applications/zephys.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/xfce4/helpers/zephys.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/nemo/actions/zephys.nemo_action
	rm -f $(DESTDIR)$(PREFIX)/share/kio/servicemenus/zephys.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/file-manager/actions/zephys.desktop

uninstall-user:
	rm -f $(HOME)/.local/bin/$(BIN)
	rm -f $(HOME)/.local/share/applications/zephys.desktop
	rm -f $(HOME)/.local/share/xfce4/helpers/zephys.desktop
	rm -f $(HOME)/.local/share/nemo/actions/zephys.nemo_action
	rm -f $(HOME)/.local/share/kio/servicemenus/zephys.desktop
	rm -f "$(HOME)/.local/share/nautilus/scripts/Open in Zephys"
	rm -f $(HOME)/.local/share/file-manager/actions/zephys.desktop


.PHONY: all debug check-deps clean install install-user uninstall uninstall-user

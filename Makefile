# NeDE — Efficient Desktop Environment
# Main Makefile

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
WAYLAND_SESSIONS = $(PREFIX)/share/wayland-sessions

VERSION = 0.1.0
PACKAGE = nede-$(VERSION)

all: components

components:
	@echo "=== Building NedePanel ==="
	@$(MAKE) -C panel
	@echo ""
	@echo "=== Building NeDE Desktop ==="
	@$(MAKE) -C desktop
	@echo ""
	@echo "=== Building NeDE ScreenShot ==="
	@$(MAKE) -C nedeshot
	@echo ""
	@echo "=== Building NeDE UAC ==="
	@$(MAKE) -C nedeuac
	@echo ""
	@echo "=== Preparing Launcher ==="
	@$(MAKE) -C launcher
	@echo ""
	@echo "✓ All components built!"

install: all
	@echo ""
	@echo "=== Installing NeDE v$(VERSION) ==="
	
	install -Dm755 panel/nedepanel              $(DESTDIR)$(BINDIR)/nedepanel
	install -Dm755 desktop/nedesktop            $(DESTDIR)$(BINDIR)/nedesktop
	install -Dm755 nedeshot/nedess              $(DESTDIR)$(BINDIR)/nedess
	install -Dm755 nedeuac/nedeuac              $(DESTDIR)$(BINDIR)/nedeuac
	install -Dm755 launcher/nedelauncher        $(DESTDIR)$(BINDIR)/nedelauncher
	
	install -Dm644 data/wayland-sessions/nede.desktop \
		$(DESTDIR)$(WAYLAND_SESSIONS)/nede.desktop
	
	@echo ""
	@echo "✓ NeDE v$(VERSION) installed successfully!"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Copy config/labwc/autostart to ~/.config/labwc/"
	@echo "  2. Copy config/pam/polkit-1 to /etc/pam.d/"
	@echo "  3. Restart LightDM or relogin"

uninstall:
	@echo "=== Uninstalling NeDE ==="
	rm -f $(DESTDIR)$(BINDIR)/nedepanel
	rm -f $(DESTDIR)$(BINDIR)/nedesktop
	rm -f $(DESTDIR)$(BINDIR)/nedess
	rm -f $(DESTDIR)$(BINDIR)/nedeuac
	rm -f $(DESTDIR)$(BINDIR)/nedelauncher
	rm -f $(DESTDIR)$(WAYLAND_SESSIONS)/nede.desktop
	@echo "✓ NeDE uninstalled"

clean:
	@echo "=== Cleaning ==="
	@$(MAKE) -C panel clean
	@$(MAKE) -C desktop clean
	@$(MAKE) -C nedeshot clean
	@$(MAKE) -C nedeuac clean
	@$(MAKE) -C launcher clean

dist: clean
	@mkdir -p dist
	@tar czf dist/$(PACKAGE).tar.gz \
		--exclude='.git' \
		--exclude='dist' \
		--exclude='*.o' \
		--exclude='panel/nedepanel' \
		--exclude='desktop/nedesktop' \
		--exclude='nedeshot/nedess' \
		--exclude='nedeuac/nedeuac' \
		.
	@echo "✓ Package: dist/$(PACKAGE).tar.gz"

.PHONY: all components install uninstall clean dist

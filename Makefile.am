AUTOMAKE_OPTIONS = foreign
SUBDIRS = po libgame src 
EXTRA_DIST = m4  gettext.h config.rpath \
	TODO COPYING README Changelog CROSSCOMPILE \
	lbreakouthd.desktop lbreakouthd256.png lbreakouthd48.png

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/icons/hicolor/256x256/apps
	$(mkinstalldirs) $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps
	$(mkinstalldirs) $(DESTDIR)$(datadir)/applications
	$(INSTALL_DATA) lbreakouthd256.png $(DESTDIR)$(datadir)/icons/hicolor/256x256/apps/lbreakouthd.png
	$(INSTALL_DATA) lbreakouthd48.png $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/lbreakouthd.png
	$(INSTALL_DATA) lbreakouthd.desktop $(DESTDIR)$(datadir)/applications
	

uninstall-local:
	rm $(DESTDIR)$(datadir)/icons/hicolor/256x256/apps/lbreakouthd.png
	rm $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/lbreakouthd.png
	rm $(DESTDIR)$(datadir)/applications/lbreakouthd.desktop

ACLOCAL_AMFLAGS = -I m4

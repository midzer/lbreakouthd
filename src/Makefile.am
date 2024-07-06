bin_PROGRAMS = lbreakouthd
lbreakouthd_SOURCES = sdl.cpp sdl.h view.h view.cpp \
	tools.h tools.cpp main.cpp clientgame.h clientgame.cpp \
	config.cpp config.h hiscores.cpp hiscores.h theme.cpp theme.h \
	sprite.cpp sprite.h mixer.h menu.cpp menu.h selectdlg.h selectdlg.cpp \
	editor.h editor.cpp
lbreakouthd_LDADD = ../libgame/libgame.a

EXTRA_DIST = empty.hscr

SUBDIRS = levels themes

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)
	$(mkinstalldirs) $(DESTDIR)$(hiscoredir)
	if ! test -f $(DESTDIR)$(hiscoredir)/lbreakouthd.hscr; then \
	  $(INSTALL_DATA) -m 666 empty.hscr $(DESTDIR)$(hiscoredir)/lbreakouthd.hscr; \
	fi;

uninstall-local:
	rm -rf $(DESTDIR)$(datadir)/lbreakouthd
	rm $(DESTDIR)$(hiscoredir)/lbreakouthd.hscr

clean:
	rm -f *.o lbreakouthd

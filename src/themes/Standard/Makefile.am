EXTRA_DIST = back0.jpg back1.jpg back2.jpg back3.jpg back4.jpg \
CREDITS SIL-OFL.txt shot.png ball.png paddle.png weapon.png \
bricks.png extras.png life.png fsmall.otf fnormal.otf theme.ini \
menuback.jpg explosions.png frame.png warp.png shine.png \
attach.wav       energyhit.wav  goldshower.wav     shot.wav \
bonusmagnet.wav  excellent.wav  joker.wav          shrink.wav \
brickhit.wav     expand.wav     looselife.wav      speeddown.wav \
chaos.wav        explball.wav   malusmagnet.wav    speedup.wav \
click.wav        explosion.wav  menuclick.wav      standard.wav \
dammit.wav       extraball.wav  menumotion.wav     timeadd.wav \
damn.wav         freeze.wav     reflectbrick.wav   verygood.wav \
darkness.wav     gainlife.wav   reflectpaddle.wav  wall.wav \
disable.wav      ghost.wav      score.wav          weakball.wav

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)/themes/Standard
	@for file in $(EXTRA_DIST); do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(datdir)/themes/Standard/$$file;\
	done

KOBOGFX_OBJECTS = *.png *.pcx
#	loading.png	\
#	bigship.pcx	\
#	brushes.png	\
#	bullets.pcx	\
#	counter.png	\
#	font2b.png	\
#	font3b.png	\
#	font4b.png	\
#	logo3.png	\
#	screen.png	\
#	sprites.pcx	\
#	tiles.pcx	\
#	tube1.png	\
#	tube2.png

KOBOSFX_OBJECTS = *.raw *.agw *.mid *.h
#	bgm.raw		\
#	bubble.raw	\
#	expl.raw	\
#	expl2.raw	\
#	expl3.raw	\
#	oneup.raw	\
#	shot.raw	\
#	metallic.raw	\
#	fine.raw	\
#	beam.raw	\
#	ring.raw	\
#	enemym.raw	\
#	bomb.raw	\
#	*.agw		\
#	*.mid		\
#	*.h

if BUILD_MACOSX_BUNDLE
kobogfxdir = $(srcdir)/../@PACKAGE@.app/Contents/Resources/gfx
kobosfxdir = $(srcdir)/../@PACKAGE@.app/Contents/Resources/sfx
else

if BUILD_SIMPLE_BUNDLE
kobogfxdir = $(srcdir)/../@PACKAGE@/gfx
kobosfxdir = $(srcdir)/../@PACKAGE@/sfx
else
kobogfxdir = $(datadir)/games/kobo-deluxe/gfx
kobosfxdir = $(datadir)/games/kobo-deluxe/sfx
endif

endif

kobogfx_DATA = $(KOBOGFX_OBJECTS)
kobosfx_DATA = $(KOBOSFX_OBJECTS)

EXTRA_DIST = $(KOBOGFX_OBJECTS) $(KOBOSFX_OBJECTS)

if UNIX_SCOREDIR
koboscoredir = @SCOREDIR@
koboscore_DATA = 
else
koboscoredir = $(srcdir)/../@PACKAGE@/scores
koboscore_DATA = 
endif

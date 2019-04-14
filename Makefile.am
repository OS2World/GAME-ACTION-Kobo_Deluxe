if BUILD_MACOSX_BUNDLE
bundle_contents = @PACKAGE@.app/Contents
else
if BUILD_SIMPLE_BUNDLE
bundle_contents = @PACKAGE@
endif
endif

kobo_scoredir=@SCOREDIR@

INCLUDES =	-DKOBO_DATA_DIR=\""@DATADIR@"\"		\
		-DKOBO_SCORE_DIR=\""$(kobo_scoredir)"\"	\
		-DKOBO_CONFIG_DIR=\""@CONFIGDIR@"\"	\
		-DKOBO_CONFIG_FILE=\""@CONFIGFILE@"\"	\
		-DSYSCONF_DIR=\""$(sysconfdir)"\"	\
		-Isound -Igraphics -Ieel -Idata @LOCALCFLAGS@

SUBDIRS = graphics sound data eel

bin_PROGRAMS = kobodl

GRAPHICS_LIB = graphics/libgraphics.a @GL_LIBS@ @MATHLIB@
SOUND_LIB = sound/libsound.a @AUDIO_LIBS@
EEL_LIB = eel/libeel.a

kobodl_SOURCES = enemies.cpp enemy.cpp pfile.cpp gamectl.cpp manage.cpp map.cpp	\
		myship.cpp radar.cpp random.cpp scenes.cpp score.cpp screen.cpp	\
		filemap.cpp prefs.cpp cfgform.cpp options.cpp gamestate.cpp	\
		states.cpp form.cpp cfgparse.cpp game.cpp kobo.cpp
kobodl_LDADD = $(GRAPHICS_LIB) $(SOUND_LIB) $(EEL_LIB)

kobodl_LDFLAGS = -o $(EXEFILE)

CLEANFILES = $(EXEFILE) Info.plist

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = *.h Info.plist.in 501 COPYING.LIB	\
		README.jp README.sfont README.xkobo README.xkobo.jp	\
		cfg-work cfg-beta cfg-release buildpkg.sh


if BUILD_MACOSX_BUNDLE

@PACKAGE@_bundle: ${EXEFILE}
	mkdir -p $(bundle_contents)/MacOS
	echo "APPL????" > $(bundle_contents)/PkgInfo
	$(INSTALL_DATA) Info.plist $(bundle_contents)
	$(INSTALL_PROGRAM) ${EXEFILE} $(bundle_contents)/MacOS/

install-exec-hook: @PACKAGE@_bundle
	rm -rf $(DESTDIR)$(prefix)/@PACKAGE@.app
	mkdir -p $(DESTDIR)$(prefix)/
	cp -r @PACKAGE@.app $(DESTDIR)$(prefix)/

clean-local:
	rm -rf $(bundle_contents)

else

if BUILD_SIMPLE_BUNDLE

@PACKAGE@_bundle: ${EXEFILE}
	$(INSTALL_PROGRAM) ${EXEFILE} $(bundle_contents)/

install-exec-hook: @PACKAGE@_bundle
	rm -rf $(DESTDIR)/$(prefix)/@PACKAGE@
	mkdir -p $(DESTDIR)/$(prefix)/
	cp COPYING @PACKAGE@/COPYING.txt
	cp COPYING.LIB @PACKAGE@/COPYING_LIB.txt
	cp ChangeLog @PACKAGE@/ChangeLog.txt
	cp TODO @PACKAGE@/TODO.txt
	cp README @PACKAGE@/README.txt
	cp README.jp @PACKAGE@/README_jp.txt
	cp README.sfont @PACKAGE@/sfont.txt
	cp README.xkobo @PACKAGE@/xkobo.txt
	cp README.xkobo.jp @PACKAGE@/xkobo_jp.txt
	cp 501 @PACKAGE@/scores/42
	cp -r @PACKAGE@	 $(DESTDIR)/$(prefix)/

clean-local:
	rm -rf $(bundle_contents)

else
@PACKAGE@_bundle:

install-exec-hook:

clean-local:

endif

endif


if UNIX_SCOREDIR

install-data-hook:	$(DESTDIR)$(kobo_scoredir)
	chmod a+w $(DESTDIR)$(kobo_scoredir)

else

install-data-hook:

endif

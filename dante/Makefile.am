AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = @TOPSUBDIRS@
DIST_SUBDIRS = @DISTSUBDIRS@

EXTRA_DIST = BUGS CREDITS LICENSE SUPPORT VERSION VERSION.in UPGRADE \
	     README.ldap @TOPEXTRADIST@ \
	     ci/run-server-tests.sh ci/smoke.py \
	     docs/plans/logformat-json.md \
	     docs/features/logformat-json/README.md \
	     docs/features/logformat-json/acceptance.md

#XXX hack used to ensure depcomp is rerun when Makefile.am files are changed
_reconf: $(top_srcdir)/lib/Makefile.am $(top_srcdir)/dlib/Makefile.am $(top_srcdir)/sockd/Makefile.am $(top_srcdir)/unlicensed/Makefile.am
	./configure `config.status --config`

#for unit tests, list subdirs for unit testing
dumpunitsubdirs:
	@echo $(UNITSUBDIRS)

# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007
#    NOVELL (All rights reserved)
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Novell, Inc.
# ----------------------------------------------------------------------
NAME=apparmor-parser
all:
COMMONDIR=../common/

include common/Make.rules

COMMONDIR_EXISTS=$(strip $(shell [ -d ${COMMONDIR} ] && echo true))
ifeq ($(COMMONDIR_EXISTS), true)
common/Make.rules: $(COMMONDIR)/Make.rules
	ln -sf $(COMMONDIR) .
endif

DESTDIR=/
APPARMOR_BIN_PREFIX=${DESTDIR}/lib/apparmor
CONFDIR=/etc/apparmor
INSTALL_CONFDIR=${DESTDIR}${CONFDIR}
LOCALEDIR=/usr/share/locale
MANPAGES=apparmor.d.5 apparmor.7 apparmor_parser.8 subdomain.conf.5 apparmor.vim.5

YACC	:= /usr/bin/bison
YFLAGS	:= -d
LEX	:= /usr/bin/flex
LEXFLAGS = -B -v
WARNINGS = -Wall
EXTRA_WARNINGS = -Wsign-compare -Wmissing-field-initializers -Wformat-security -Wunused-parameter
CXX_WARNINGS = ${WARNINGS} $(shell for warning in ${EXTRA_WARNINGS} ; do \
			if ${CC} $${warning} -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then \
				echo "$${warning}"; \
			fi ; \
		done)
CPP_WARNINGS = -Wstrict-prototypes -Wnested-externs
ifndef CFLAGS
CFLAGS	= -g -O2 -pipe

ifdef DEBUG
CFLAGS += -pg -D DEBUG
endif
endif #CFLAGS

EXTRA_CXXFLAGS = ${CFLAGS} ${CXX_WARNINGS} -D_GNU_SOURCE
EXTRA_CFLAGS = ${EXTRA_CXXFLAGS} ${CPP_WARNINGS}

#LEXLIB	:= -lfl

# override this on the make command to point to where the immunix.h file is
# (yeah this is lame, but since we are tied to the kernel so tightly...)
#INCLUDEDIR = /usr/src/linux/include
INCLUDEDIR =

ifdef INCLUDEDIR
	CFLAGS += -I$(INCLUDEDIR)
endif

# Internationalization support. Define a package and a LOCALEDIR
EXTRA_CFLAGS+=-DPACKAGE=\"${NAME}\" -DLOCALEDIR=\"${LOCALEDIR}\"

# Compile-time configuration of the location of the config file
EXTRA_CFLAGS+=-DSUBDOMAIN_CONFDIR=\"${CONFDIR}\"

SRCS = parser_common.c parser_include.c parser_interface.c parser_lex.c \
       parser_main.c parser_misc.c parser_merge.c parser_symtab.c \
       parser_yacc.c parser_regex.c parser_variable.c parser_policy.c \
       parser_alias.c mount.c
HDRS = parser.h parser_include.h immunix.h mount.h
TOOLS = apparmor_parser

OBJECTS = $(SRCS:.c=.o)

AAREDIR= libapparmor_re
AAREOBJECT = ${AAREDIR}/libapparmor_re.a
AAREOBJECTS = $(AAREOBJECT) libstdc++.a
AARE_LDFLAGS=-static-libgcc -L.

LEX_C_FILES	= parser_lex.c
YACC_C_FILES	= parser_yacc.c parser_yacc.h

TESTS = tst_regex tst_misc tst_symtab tst_variable
TEST_CFLAGS = $(EXTRA_CFLAGS) -DUNIT_TEST -Wno-unused-result
TEST_OBJECTS = $(filter-out \
			parser_lex.o \
			parser_yacc.o \
			parser_main.o, ${OBJECTS}) \
               $(AAREOBJECTS)
TEST_LDFLAGS = $(AARE_LDFLAGS)

ifdef V
  VERBOSE = 1
endif
ifndef VERBOSE
  VERBOSE = 0
endif
ifeq ($(VERBOSE),1)
  BUILD_OUTPUT =
  Q =
else
  BUILD_OUTPUT = > /dev/null 2>&1
  Q = @
endif
export Q VERBOSE BUILD_OUTPUT

po/${NAME}.pot: ${SRCS} ${HDRS}
	$(MAKE) -C po ${NAME}.pot NAME=${NAME} SOURCES="${SRCS} ${HDRS}"

techdoc.pdf: techdoc.tex
	timestamp=$(shell date "+%Y%m%d%H%M%S+02'00'" -r $< );\
	while pdflatex "\def\fixedpdfdate{$$timestamp}\input $<" ${BUILD_OUTPUT} || exit 1 ; \
		grep -q "Label(s) may have changed" techdoc.log; \
	do :; done

techdoc/index.html: techdoc.pdf
	latex2html -show_section_numbers -split 0 -noinfo -nonavigation -noaddress techdoc.tex ${BUILD_OUTPUT}

techdoc.txt: techdoc/index.html
	w3m -dump $< > $@

# targets arranged this way so that people who don't want full docs can
# pick specific targets they want.
arch: 	$(TOOLS)

manpages:	$(MANPAGES)

htmlmanpages:	$(HTMLMANPAGES)

pdf:	techdoc.pdf

docs:	manpages htmlmanpages pdf

indep: docs
	$(Q)$(MAKE) -C po all

all:	arch indep


.PHONY: libstdc++.a
libstdc++.a:
	rm -f ./libstdc++.a
	ln -s `$(CXX) -print-file-name=libstdc++.a`

apparmor_parser: $(OBJECTS) $(AAREOBJECTS)
	$(CXX) $(EXTRA_CFLAGS) -o $@ $(OBJECTS) $(LIBS) \
	      ${LEXLIB}  $(AAREOBJECTS) $(AARE_LDFLAGS)

parser_yacc.c parser_yacc.h: parser_yacc.y parser.h
	$(YACC) $(YFLAGS) -o parser_yacc.c parser_yacc.y

parser_lex.c: parser_lex.l parser_yacc.h parser.h
	$(LEX) ${LEXFLAGS} -o$@ $<

parser_lex.o: parser_lex.c parser.h parser_yacc.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_misc.o: parser_misc.c parser.h parser_yacc.h af_names.h cap_names.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_yacc.o: parser_yacc.c parser_yacc.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_main.o: parser_main.c parser.h parser_version.h libapparmor_re/apparmor_re.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_interface.o: parser_interface.c parser.h libapparmor_re/apparmor_re.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_include.o: parser_include.c parser.h parser_include.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_merge.o: parser_merge.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_regex.o: parser_regex.c parser.h libapparmor_re/apparmor_re.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_symtab.o: parser_symtab.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_variable.o: parser_variable.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_policy.o: parser_policy.c parser.h parser_yacc.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_alias.o: parser_alias.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_common.o: parser_common.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

mount.o: mount.c mount.h parser.h immunix.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_version.h: Makefile
	@echo \#define PARSER_VERSION \"$(VERSION)\" > .ver
	@mv -f .ver $@

# af_names and capabilities generation has moved to common/Make.rules,
# as well as the filtering that occurs for network protocols that
# apparmor should not mediate.

.PHONY: af_names.h
af_names.h:
	echo "$(AF_NAMES)" | LC_ALL=C sed -n -e 's/[ \t]\?AF_MAX[ \t]\+[0-9]\+,//g'  -e 's/[ \t]\+\?AF_\([A-Z0-9_]\+\)[ \t]\+\([0-9]\+\),/#ifndef AF_\1\n#  define AF_\1 \2\n#endif\nAA_GEN_NET_ENT("\L\1", \UAF_\1)\n\n/pg' > $@
	echo "$(AF_NAMES)" | LC_ALL=C sed -n -e 's/.*,[ \t]\+AF_MAX[ \t]\+\([0-9]\+\),\?.*/#define AA_AF_MAX \1\n/p' >> $@
	# cat $@

cap_names.h: /usr/include/linux/capability.h
	echo "$(CAPABILITIES)" | LC_ALL=C sed -n -e "s/[ \\t]\\?CAP_\\([A-Z0-9_]\\+\\)/\{\"\\L\\1\", \\UCAP_\\1\},\\n/pg" > $@

tst_%: parser_%.c parser.h $(filter-out parser_%.o, ${TEST_OBJECTS})
	$(CC) $(TEST_CFLAGS) -o $@ $< $(filter-out $(<:.c=.o), ${TEST_OBJECTS}) $(TEST_LDFLAGS)

.SILENT: check
.PHONY: check
check: tests

.SILENT: tests
tests: apparmor_parser ${TESTS}
	sh -e -c 'for test in ${TESTS} ; do echo "*** running $${test}" && ./$${test}; done'
	$(Q)$(MAKE) -s -C tst tests

# always need to rebuild.
.SILENT: $(AAREOBJECT)
.PHONY: $(AAREOBJECT)
$(AAREOBJECT):
	$(MAKE) -C $(AAREDIR) CFLAGS="$(EXTRA_CXXFLAGS)"

.PHONY: install-rhel4
install-rhel4: install-redhat

.PHONY: install-redhat
install-redhat:
	install -m 755 -d $(DESTDIR)/etc/init.d
	install -m 755 rc.apparmor.$(subst install-,,$@) $(DESTDIR)/etc/init.d/apparmor

.PHONY: install-suse
install-suse:
	install -m 755 -d $(DESTDIR)/etc/init.d
	install -m 755 rc.apparmor.$(subst install-,,$(@)) $(DESTDIR)/etc/init.d/boot.apparmor
	install -m 755 -d $(DESTDIR)/sbin
	ln -sf /etc/init.d/boot.apparmor $(DESTDIR)/sbin/rcapparmor
	ln -sf rcapparmor $(DESTDIR)/sbin/rcsubdomain

.PHONY: install-slackware
install-slackware:
	install -m 755 -d $(APPARMOR_BIN_PREFIX)/install
	install -m 755 frob_slack_rc $(APPARMOR_BIN_PREFIX)/install
	install -m 755 -d $(DESTDIR)/etc/rc.d
	install -m 755 rc.apparmor.$(subst install-,,$(@)) $(DESTDIR)/etc/rc.d/rc.apparmor

.PHONY: install-debian
install-debian:

.PHONY: install-unknown
install-unknown:

INSTALLDEPS=arch
ifdef DISTRO
INSTALLDEPS+=install-$(DISTRO)
endif

.PHONY: install
install: install-indep install-arch

.PHONY: install-arch
install-arch: $(INSTALLDEPS)
	install -m 755 -d $(DESTDIR)/sbin
	install -m 755 ${TOOLS} $(DESTDIR)/sbin

.PHONY: install-indep
install-indep:
	install -m 755 -d $(INSTALL_CONFDIR)
	install -m 644 subdomain.conf $(INSTALL_CONFDIR)
	install -m 644 parser.conf $(INSTALL_CONFDIR)
	install -m 755 -d ${DESTDIR}/var/lib/apparmor
	install -m 755 -d $(APPARMOR_BIN_PREFIX)
	install -m 755 rc.apparmor.functions $(APPARMOR_BIN_PREFIX)
	$(MAKE) -C po install NAME=${NAME} DESTDIR=${DESTDIR}
	$(MAKE) install_manpages DESTDIR=${DESTDIR}

.SILENT: clean
.PHONY: clean
clean: _clean
	rm -f core core.* *.o *.s *.a *~
	rm -f $(TOOLS) $(TESTS)
	rm -f $(LEX_C_FILES)
	rm -f $(YACC_C_FILES)
	rm -f parser_version.h
	rm -f $(NAME)*.tar.gz $(NAME)*.tgz
	rm -f af_names.h
	rm -f cap_names.h
	rm -rf techdoc.aux techdoc.out techdoc.log techdoc.pdf techdoc.toc techdoc.txt techdoc/
	$(MAKE) -s -C $(AAREDIR) clean
	$(MAKE) -s -C po clean
	$(MAKE) -s -C tst clean

.SILENT: dist_clean
dist_clean:
	@$(MAKE) clean
	@rm -f $(LEX_C_FILES) $(YACC_C_FILES)

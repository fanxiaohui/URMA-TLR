## Process this file with automake to produce Makefile.in

POD2MAN = pod2man

man_MANS = aa_change_hat.2 aa_change_profile.2 aa_getcon.2 aa_find_mountpoint.2

PODS = $(subst .2,.pod,$(man_MANS))

EXTRA_DIST = $(man_MANS) $(PODS)

## delete man pages at maintainer-clean
BUILT_SOURCES = $(man_MANS)

%.2: %.pod
	$(POD2MAN) \
		--section=2 \
		--release="AppArmor $(VERSION)" \
		--center="AppArmor" \
		--stderr \
		$< > $@

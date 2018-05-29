#
# Toplevel makefile for OS/161.
#
#
# Main rules:
#    all (default):  depend and compile system; install into staging area
#    rebuild:        likewise, but start with a clean slate.
#    fullrebuild:    likewise, but start with a very clean slate.
#
# What all does, in order:
#    tools:          depend and compile the tools used in build.
#    includes:       install header files.
#    build:          depend and compile the system.
#
# Other targets:
#    depend:         just update make dependency information.
#    tags:           generate/regenerate "tags" files.
#    install:        install into $(OSTREE).
#    clean:          remove generated files.
#    distclean:      remove all generated files.
#

TOP=.
.include "$(TOP)/mk/os161.config.mk"

all:;  # make this first

MKDIRS=$(OSTREE)

.include "$(TOP)/mk/os161.mkdirs.mk"

all: tools .WAIT includes .WAIT build

rebuild:
	$(MAKE) clean
	$(MAKE) all

fullrebuild:
	$(MAKE) distclean
	$(MAKE) all

# currently no tools required, hence no tools/ dir or work to do
tools:
	@true

build:
	(cd userland && $(MAKE) build)
	(cd man && $(MAKE) install-staging)
	(cd testscripts && $(MAKE) build)

includes tags depend:
	(cd kern && $(MAKE) $@)
	(cd userland && $(MAKE) $@)

clean:
	(cd kern && $(MAKE) $@)
	(cd userland && $(MAKE) $@)
	rm -rf $(INSTALLTOP)

distclean: clean
	rm -rf $(WORKDIR)

install: $(OSTREE)
	(cd $(INSTALLTOP) && tar -cf - .) | (cd $(OSTREE) && tar -xvf -)


.PHONY: all rebuild fullrebuild tools build includes tags depend
.PHONY: clean distclean

# old BSD name, same as distclean
cleandir: distclean
.PHONY: cleandir

k: # Build kernel
	cd kern/compile/ASST3 && bmake && bmake install
u: # Build userland
	bmake && bmake install

a:
	bmake k && bmake u

q:
	bmake k > /dev/null && bmake u > /dev/null

run: # Run kernel
	cd ../root && sys161 kernel #"p /bin/true"
	# cd ../root && sys161 kernel "$(args)"

conf: # Full reconfigure
	./configure && \
	bmake && bmake install && \
	cd kern/conf && ./config ASST3 && \
	cd ../compile/ASST3 && bmake depend && bmake && bmake install

domconf: # CONF WITH DOM'S SILLY DIRECTORY SYSTEM
	./configure --ostree='~/Documents/comp3231/assignments/root' && \
	bmake && bmake install && \
	cd kern/conf && ./config ASST3 && \
	cd ../compile/ASST3 && bmake depend && bmake && bmake install

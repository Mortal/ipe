# --------------------------------------------------------------------
# Makefile for building all components of Ipe
# --------------------------------------------------------------------

GOAL ?= all

subdirs = \
	ipelib \
	ipelets/image \
	ipelets/lua \
	ipetoipe \
	ipe6upgrade \
	ipeextract \
	ipecairo \
	iperender \
	ipeqtcanvas \
	ipeview \
	ipelua \
	ipeui \
	ipe

.PHONY: all $(subdirs)

all: $(subdirs)

ipetoipe: ipelib
ipelets/kgon: ipelib
ipelets/image: ipelib
ipelets/qvoronoi: ipelib
ipe6update: ipelib
ipeextract: ipelib
ipecairo: ipelib
iperender: ipelib ipecairo
ipeqtcanvas: ipelib ipecairo
ipeview: ipeqtcanvas
ipelua: ipelib
ipe: ipeqtcanvas ipelua

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

.PHONY: clean install
clean:
	$(MAKE)	GOAL=clean

install: all
	$(MAKE) GOAL=install

# --------------------------------------------------------------------

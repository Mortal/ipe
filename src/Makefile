# --------------------------------------------------------------------
# Makefile for building all components of Ipe
# --------------------------------------------------------------------

GOAL ?= all

subdirs = \
	ipelib \
	ipelets/lua \
	ipelets/image \
	ipetoipe \
	ipe6upgrade \
	ipeextract \
	ipescript \
	ipecairo \
	iperender \
	ipecanvas \
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
ipe6upgrade: ipelib
ipeextract: ipelib
ipecairo: ipelib
iperender: ipelib ipecairo
ipecanvas: ipelib ipecairo
ipeview: ipecanvas
ipelua: ipelib
ipescript: ipelib ipelua
ipe: ipecanvas ipelua

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

.PHONY: clean install
clean:
	$(MAKE)	GOAL=clean

install: all
	$(MAKE) GOAL=install

# --------------------------------------------------------------------

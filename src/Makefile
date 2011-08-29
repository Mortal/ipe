# --------------------------------------------------------------------
# Makefile for building all components of Ipe
# --------------------------------------------------------------------

GOAL ?= all

subdirs = \
	ipelib \
	ipelets/lua \
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

#	ipelets/image \

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
ipeqtcanvas: ipelib ipecairo
ipeview: ipeqtcanvas
ipelua: ipelib
ipescript: ipelib ipelua
ipe: ipeqtcanvas ipelua

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

.PHONY: clean install
clean:
	$(MAKE)	GOAL=clean

install: all
	$(MAKE) GOAL=install

# --------------------------------------------------------------------

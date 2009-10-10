# --------------------------------------------------------------------
# Makefile for Lua ipelets (only for install)
# --------------------------------------------------------------------

IPESRCDIR = ../..
include ../../common.mak

LUASCRIPTS = move.lua goodies.lua align.lua euclid.lua \
	symbols.lua search-replace.lua

all: 

install: 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELETDIR)
	$(INSTALL_FILES) $(INSTALL_ROOT)$(IPELETDIR) $(LUASCRIPTS)

# --------------------------------------------------------------------

#  Copyright 2005 Adam Jackson.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AUTOMAKE_OPTIONS = foreign
SUBDIRS = src

EXTRA_DIST = misc/M2012-0.0.4.rom misc/M2012-0.0.8.rom misc/00-rdc.conf

# Xorg reads user config snippets from /etc/X11/xorg.conf.d regardless of the
# build's sysconfdir (e.g. AOSC uses /usr/etc but Xorg still reads /etc).
XORG_CONFIG_DIR = /etc/X11/xorg.conf.d

# The driver reads its VBIOS firmware from this fixed directory (see
# BIOS_ROM_PATH_DIR in rdc.h); Option "VBIOS" selects which file to use.
FIRMWARE_DIR = /usr/lib/firmware-rdc
RDC_VBIOS_FILES = M2012-0.0.4.rom M2012-0.0.8.rom

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(FIRMWARE_DIR)
	for f in $(RDC_VBIOS_FILES); do \
		$(INSTALL_DATA) $(srcdir)/misc/$$f $(DESTDIR)$(FIRMWARE_DIR)/$$f; \
	done
	@if test ! -f $(DESTDIR)$(XORG_CONFIG_DIR)/00-rdc.conf; then \
		echo "  Installing xorg.conf.d/00-rdc.conf to $(XORG_CONFIG_DIR)"; \
		$(MKDIR_P) $(DESTDIR)$(XORG_CONFIG_DIR); \
		$(INSTALL_DATA) $(srcdir)/misc/00-rdc.conf $(DESTDIR)$(XORG_CONFIG_DIR)/00-rdc.conf; \
	else \
		echo "  $(XORG_CONFIG_DIR)/00-rdc.conf already exists, not overwriting"; \
	fi

uninstall-local:
	for f in $(RDC_VBIOS_FILES); do \
		rm -f $(DESTDIR)$(FIRMWARE_DIR)/$$f; \
	done
	rmdir $(DESTDIR)$(FIRMWARE_DIR) 2>/dev/null || true
	rm -f $(DESTDIR)$(XORG_CONFIG_DIR)/00-rdc.conf

clean-local:
	rm -f Makefile Makefile.in aclocal.m4 compile config.guess config.h config.h.in config.log config.status config.sub configure depcomp install-sh libtool ltmain.sh missing stamp-h1
	rm -rf autom4te.cache
	rm -f src/rdcm15_drv.la
	rm -rf src/.deps src/.libs
	rm -f src/*.o src/*.lo
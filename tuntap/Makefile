# Lets have a version, at last!
TUNTAP_VERSION = 20070920

# BASE install directory
BASE=

all:	tap.kext tun.kext
	cp -pR tap.kext pkg/tap_kext/
	cp -pR tun.kext pkg/tun_kext/
	cp -pR startup_item/tap pkg/startup_item/
	cp -pR startup_item/tun pkg/startup_item/

pkg:	all
	sed -e 's/@@TUNTAP_VERSION@@/$(TUNTAP_VERSION)/' pkg/tap_kext.desc/Info.plist.in > pkg/tap_kext.desc/Info.plist
	sed -e 's/@@TUNTAP_VERSION@@/$(TUNTAP_VERSION)/' pkg/tun_kext.desc/Info.plist.in > pkg/tun_kext.desc/Info.plist
	sed -e 's/@@TUNTAP_VERSION@@/$(TUNTAP_VERSION)/' pkg/startup_item.desc/Info.plist.in > pkg/startup_item.desc/Info.plist
	tools/makepkg.sh pkg/tap_kext.pkg pkg/tap_kext pkg/tap_kext.res pkg/tap_kext.desc
	tools/makepkg.sh pkg/tun_kext.pkg pkg/tun_kext pkg/tun_kext.res pkg/tun_kext.desc
	tools/makepkg.sh pkg/startup_item.pkg pkg/startup_item pkg/startup_item.res pkg/startup_item.desc
	-rm -rf pkg/tuntap_installer.mpkg
	cp -R pkg/tuntap.mpkg pkg/tuntap_installer.mpkg
	tar czf tuntap_leopard_$(TUNTAP_VERSION).tar.gz README.installer README -C pkg tap_kext.pkg tun_kext.pkg startup_item.pkg tuntap_installer.mpkg
	rm -rf pkg/tuntap_installer.mpkg

# Install targets
# They are provided for the gentoo ebuild, but should work just fine for other people as well.
install_kext: tap.kext tun.kext
	mkdir -p ${BASE}/Library/Extensions
	cp -pR tap.kext ${BASE}/Library/Extensions/
	chown -R root:wheel ${BASE}/Library/Extensions/tap.kext
	cp -pR tun.kext ${BASE}/Library/Extensions/ 
	chown -R root:wheel ${BASE}/Library/Extensions/tun.kext

install_scripts:
	mkdir -p ${BASE}/Library/StartupItems
	cp -pR startup_item/tap ${BASE}/Library/StartupItems/
	chown -R root:wheel ${BASE}/Library/StartupItems/tap
	cp -pR startup_item/tun ${BASE}/Library/StartupItems/ 
	chown -R root:wheel ${BASE}/Library/StartupItems/tun

install: install_kext install_scripts

tarball:	clean
	tar czf tuntap_leopard_$(TUNTAP_VERSION)_src.tar.gz --exclude "*/CVS*" --exclude "tuntap/tuntap_*src.tar.gz" \
		--exclude "*/.*" -C .. tuntap

clean:
	cd src/tap && make -f Makefile clean
	cd src/tun && make -f Makefile clean
	-rm -rf pkg/tap_kext.desc/Info.plist
	-rm -rf pkg/tun_kext.desc/Info.plist
	-rm -rf pkg/startup_item.desc/Info.plist
	-rm -rf pkg/tap_kext/tap.kext
	-rm -rf pkg/tun_kext/tun.kext
	-rm -rf pkg/startup_item/tap
	-rm -rf pkg/startup_item/tun
	-rm -rf pkg/tap_kext.pkg
	-rm -rf pkg/tun_kext.pkg
	-rm -rf pkg/startup_item.pkg
	-rm -rf tuntap_leopard_$(TUNTAP_VERSION).tar.gz
	-rm -rf tuntap_leopard_$(TUNTAP_VERSION)_src.tar.gz

tap.kext:
	cd src/tap && make TUNTAP_VERSION=$(TUNTAP_VERSION) -f Makefile all

tun.kext:
	cd src/tun && make TUNTAP_VERSION=$(TUNTAP_VERSION) -f Makefile all


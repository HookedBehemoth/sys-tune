export GITHASH := $(shell git rev-parse --short HEAD)
export VERSION := 1.2.0
export API_VERSION := 3

all: overlay nxExt module

clean:
	$(MAKE) -C sys-tune/nxExt clean
	$(MAKE) -C overlay clean
	$(MAKE) -C sys-tune clean
	rm -rf dist
	rm sys-tune-*-*.zip

overlay:
	$(MAKE) -C overlay

nxExt:
	$(MAKE) -C sys-tune/nxExt

module:
	$(MAKE) -C sys-tune
	hactool -t nso sys-tune/sys-tune.nso

dist: all
	mkdir -p dist/switch/.overlays
	mkdir -p dist/atmosphere/contents/4200000000000000
	cp sys-tune/sys-tune.nsp dist/atmosphere/contents/4200000000000000/exefs.nsp
	cp overlay/sys-tune-overlay.ovl dist/switch/.overlays/
	cd dist; zip -r sys-tune-$(VERSION)-$(GITHASH).zip ./*; cd ../;

.PHONY: all overlay module

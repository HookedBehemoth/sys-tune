export GITHASH := $(shell git rev-parse --short HEAD)
export VERSION := 0.9.0
export API_VERSION := 0

all: overlay module

clean:
	$(MAKE) -C overlay clean
	$(MAKE) -C sys-tune clean
	rm -rf dist
	rm sys-tune-*-*.zip

overlay:
	$(MAKE) -C overlay

module:
	$(MAKE) -C sys-tune

dist: all
	mkdir -p dist/switch/.overlays
	mkdir -p dist/atmosphere/contents/4200000000000000/flags
	touch dist/atmosphere/contents/4200000000000000/flags/boot2.flag
	cp sys-tune/sys-tune.nsp dist/atmosphere/contents/4200000000000000/exefs.nsp
	cp sys-tune/toolbox.json dist/atmosphere/contents/4200000000000000/toolbox.json
	cp overlay/sys-tune-overlay.ovl dist/switch/.overlays/
	zip -r sys-tune-$(VERSION)-$(GITHASH).zip dist

.PHONY: all overlay module

GITHASH := $(shell git rev-parse --short HEAD)
VERSION := 0.8.0

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
	rm -rf dist
	mkdir dist
	mkdir dist/atmosphere
	mkdir dist/atmosphere/contents
	mkdir dist/atmosphere/contents/4200000000000000
	mkdir dist/atmosphere/contents/4200000000000000/flags
	touch dist/atmosphere/contents/4200000000000000/flags/boot2.flag
	cp sys-tune/sys-tune.nsp dist/atmosphere/contents/4200000000000000/exefs.nsp
	mkdir dist/switch
	mkdir dist/switch/.overlays
	cp overlay/sys-tune-overlay.ovl dist/switch/.overlays/
	zip -r sys-tune-$(VERSION)-$(GITHASH).zip dist
	rm -r dist

.PHONY: all overlay module

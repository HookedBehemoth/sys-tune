export GITHASH 		:= $(shell git rev-parse --short HEAD)
export VERSION 		:= 2.0.0
export API_VERSION 	:= 4
export WANT_FLAC 	:= 1
export WANT_MP3 	:= 1
export WANT_WAV 	:= 1

all: overlay nxExt module

clean:
	$(MAKE) -C sys-tune/nxExt clean
	$(MAKE) -C overlay clean
	$(MAKE) -C sys-tune clean
	-rm -r dist
	-rm sys-tune-*-*.zip

overlay:
	$(MAKE) -C overlay

nxExt:
	$(MAKE) -C sys-tune/nxExt

module:
	$(MAKE) -C sys-tune

dist: all
	mkdir -p dist/switch/.overlays
	mkdir -p dist/atmosphere/contents/4200000000000000/flags
	touch dist/atmosphere/contents/4200000000000000/flags/boot2.flag
	cp sys-tune/sys-tune.nsp dist/atmosphere/contents/4200000000000000/exefs.nsp
	cp overlay/sys-tune-overlay.ovl dist/switch/.overlays/
	cp sys-tune/toolbox.json dist/atmosphere/contents/4200000000000000/
	cd dist; zip -r sys-tune-$(VERSION)-$(GITHASH).zip ./**/; cd ../;
	-hactool -t nso sys-tune/sys-tune.nso

.PHONY: all overlay module

all: createIndex speedMap

.PHONY: libsuffixtools libFMD libFMD-jar createIndex speedMap scala check

scala: libFMD-jar
	sbt stage

createIndex:
	$(MAKE) -C createIndex

libFMD:
	$(MAKE) -C libFMD

libFMD-jar:
	$(MAKE) -C libFMD jar-install

libsuffixtools:
	$(MAKE) -C libsuffixtools 
	
check:
	cd libsuffixtools && $(MAKE) check
	cd libFMD && $(MAKE) check

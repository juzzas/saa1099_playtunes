
all:
	$(MAKE) -C src/rc2014
	$(MAKE) -C src/samcoupe

.PHONY clean:
	$(MAKE) -C src/rc2014 clean
	$(MAKE) -C src/samcoupe clean

TOPTARGETS := all clean install uninstall

#SUBDIRS := $(wildcard */.)
SUBDIRS := lib app

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

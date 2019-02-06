TOPTARGETS := all clean install uninstall

SUBDIRS := src

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
	
.PHONY: $(TOPTARGETS) $(SUBDIRS)

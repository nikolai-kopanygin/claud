TOPTARGETS := all install uninstall

SUBDIRS := src

clean: $(SUBDIRS)
	rm -rf doc
    
$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
	
doc:
	doxygen
	
.PHONY: $(TOPTARGETS) $(SUBDIRS)


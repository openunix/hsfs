MANDIR = /usr/share/man
CACHEDIR = /var/cache/man
install:
	install -m 0644 mount.hsfs.8 $(MANDIR)/man8/
	install -m 0644 hsfs.5 $(MANDIR)/man5/
	gzip $(MANDIR)/man8/mount.hsfs.8
	gzip $(MANDIR)/man5/hsfs.5
uninstall:
	rm $(MANDIR)/man8/mount.hsfs.8.gz -f
	rm $(CACHEDIR)/cat8/mount.hsfs.8.lzma -f
	rm $(MANDIR)/man5/hsfs.5.gz -f 
	rm $(CACHEDIR)/cat5/hsfs.5.lzma -f

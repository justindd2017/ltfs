lib_LTLIBRARIES = libtape-winfsp.la
BASENAMES = libtape-winfsp

AM_LIBTOOLFLAGS = --tag=disable-static

libtape_winfsp_la_SOURCES = spti_scsi_tape.c spti_tape.c vendor_compat.c ibm_tape.c hp_tape.c quantum_tape.c open_factor.c
libtape_winfsp_la_DEPENDENCIES = ../../../../messages/libtape_windows_spti_dat.a ../../../libltfs/libltfs.la ./libtape_winfsp_la-reed_solomon_crc.lo ./libtape_winfsp_la-crc32c_crc.lo
libtape_winfsp_la_LIBADD = ../../../libltfs/libltfs.la ./libtape_winfsp_la-reed_solomon_crc.lo ./libtape_winfsp_la-crc32c_crc.lo
libtape_winfsp_la_LDFLAGS = -avoid-version -module @AM_LDFLAGS@ -lws2_32 ../../../../messages/libtape_windows_spti_dat.a
libtape_winfsp_la_CPPFLAGS = @AM_CPPFLAGS@ @AM_EXTRA_CPPFLAGS@ -I ../../.. -I ..

vendor_compat.c:
	ln -s ../../vendor_compat.c ./vendor_compat.c

ibm_tape.c:
	ln -s ../../ibm_tape.c ./ibm_tape.c

hp_tape.c:
	ln -s ../../hp_tape.c ./hp_tape.c

quantum_tape.c:
	ln -s ../../quantum_tape.c ./quantum_tape.c

open_factor.c:
	ln -s ../../open_factor.c ./open_factor.c

clean-local:
	rm -f vendor_compat.c ibm_tape.c hp_tape.c quantum_tape.c open_factor.c

libtape_winfsp_la-reed_solomon_crc.lo: ../../reed_solomon_crc.c
	$(LIBTOOL)  --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libtape_winfsp_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) $(CRC_OPTIMIZE) -o $@ -c ../../reed_solomon_crc.c

libtape_winfsp_la-crc32c_crc.lo: ../../crc32c_crc.c
	$(LIBTOOL)  --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libtape_winfsp_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) $(CRC_OPTIMIZE) -o $@ -c ../../crc32c_crc.c

install-exec-hook:
	mkdir -p $(DESTDIR)$(libdir)/ltfs
	for f in $(lib_LTLIBRARIES); do rm -f $(DESTDIR)$(libdir)/$$f; done
	for f in $(BASENAMES); do mv $(DESTDIR)$(libdir)/$$f* $(DESTDIR)$(libdir)/ltfs; done

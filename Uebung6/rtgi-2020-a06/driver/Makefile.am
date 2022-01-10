bin_PROGRAMS = rtgi

rtgi_SOURCES = main.cpp cmdline.cpp interaction.cpp
noinst_HEADERS = interaction.h cmdline.h

rtgi_LDADD  = ../gi/libprimary-hit.a ../rt/seq/libseq-is.a
rtgi_LDADD += ../gi/libdirect.a
rtgi_LDADD += ../rt/bbvh-base/libbbvh-base.a 
rtgi_LDADD += ../libgi/libgi.a
rtgi_LDADD += $(WAND_LIBS)

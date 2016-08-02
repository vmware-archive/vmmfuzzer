noinst_LIBRARIES = libarray.a libiofuzzer.a librandom.a
libarray_a_LIBADD = $(LIBOBJS) $(ALLOCA)
libarray_a_SOURCES = ../lib/array.c
libiofuzzer_a_CPPFLAGS = -I$(top_builddir)/lib -I$(srcdir)/lib/$(host_cpu)
libiofuzzer_a_LIBADD = $(LIBOBJS) $(ALLOCA)
libiofuzzer_a_SOURCES = lib/iofuzzer.c
librandom_a_LIBADD = $(LIBOBJS) $(ALLOCA)
librandom_a_SOURCES = ../lib/random.c

bin_PROGRAMS = iofuzzer
iofuzzer_CPPFLAGS = -DPROGRAM_NAME=\"iofuzzer\" -DPROGRAM_VERSION=\"$(PACKAGE_VERSION)\" -I$(top_builddir)/lib -I$(srcdir)/lib
iofuzzer_LDADD = libarray.a libiofuzzer.a librandom.a -lm
iofuzzer_LDFLAGS = -pthread
iofuzzer_SOURCES = iofuzzer.c

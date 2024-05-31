bin_PROGRAMS = satencap satdecap

common_SOURCES = \
	utils.c \
	utils.h \
	tap.c \
	tap.h \
	udp.c \
	udp.h

satencap_SOURCES = \
	${common_SOURCES} \
	process_encap.c \
	process_encap.h \
	satencap.c

satencap_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/src/common

satencap_LDADD = \
	$(AM_LDFLAGS) \
	$(top_builddir)/src/common/libencaptunnel_common.la

satdecap_SOURCES = \
	${common_SOURCES} \
	process_decap.c \
	process_decap.h \
	satdecap.c

satdecap_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/src/common

satdecap_LDADD = \
	$(AM_LDFLAGS) \
	$(top_builddir)/src/common/libencaptunnel_common.la

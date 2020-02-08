#
# Makefile for Webbase crawler
#
# Author: Junghoo Cho (cho@cs.stanford.edu)
#
##########

#
# sources and targets
#
LIBRARY		= libcrawler.a
LIBSRCS		= url.cc fetch.cc manager.cc my_utility.cc url_queue.cc \
		  crawl_comm.cc net_mod.cc url_filter.cc \
		  putaside_queue.cc html_parser.cc \
		  robot_parser.cc \
		  bigfile.cc callback.cc dnsclient.cc readmsg.cc parameter.cc \
		  crawl_utility.cc 
BINSRCS		= crawl_server.cc webcat.cc
# BINSRCS	= crawl_server.cc webcat.cc statcrawler.cc
#		  test.cc tst4.cc \
#		  webcast.cc
SRCS		= ${LIBSRCS} ${BINSRCS}

#
# compiler configuration
#

OS		= linux
#OS		= sun
#THREADS		= yes

INCS		= -I../../port/include
#INCS		= -I/u/pranavk/project/stl/ -I/u/pranavk/project/library/
#LIBS		= /u/cho/project/zlib/${OS}/libz.a
#LIBS		= /u/cho/project/zlib/${OS}/libz.a archiver.a
# // status file - <wlam@cs.stanford.edu> Feb 2001
STATUS		= -DSOCKET_CONTROL
# BEGIN SOLARIS PORT CHANGE
# CCFLAGS		= -g -Wall ${INCS} ${STATUS}
CCFLAGS		= -g -Wall ${INCS} ${STATUS} -D_REENTRANT
# END SOLARIS PORT CHANGE
# CCFLAGS		= -g -Wall ${INCS}
# // status file - <wlam@cs.stanford.edu> Feb 2001
#CCFLAGS		= -g -Wall ${INCS} -DHISTORY -DNOSAVE
#CCFLAGS		= -g -Wall ${INCS} -DJUN_ARCHIVER
#CCFLAGS		= -g -Wall ${INCS} -DNOSAVE -DSTANFORD -DNOLOG -DHISTORY

#
# Linux
#
ifeq (${OS},linux)
   CCFLAGS	+= -DLINUX
#   INCS		+= -I/u/cho/project/bind/Linux/include
   INCS		+= -I/usr/include
#   LIBS		+= -L/u/cho/project/bind/Linux/lib -lbind -lnsl -lz
   LIBS		+= -L/usr/lib/ -lnsl -lz
   ifdef THREADS
      LIBS	+= -lpthread_p
      CCFLAGS	+= -D_REENTRANT -D_PTHREADS
   else
      LIBS	+= 
      CCFLAGS	+= 
   endif
endif

#
# Sun
#
ifeq (${OS},sun)
   CCFLAGS	+= -DSUN
   INCS		+= -I/u/cho/project/bind/SunOS/include
   LIBS		+= -L/u/cho/project/bind/SunOS/lib
   ifdef THREADS
      LIBS	+= -lbind -lsocket -lnsl -lpthread -lposix4 
      CCFLAGS	+= -D_REENTRANT -D_PTHREADS
   else
      LIBS	+= -lbind -lsocket -lnsl
      CCFLAGS	+=
   endif
endif

#
# purify configuration
#
PFLAGS		= -cache-dir=/tmp/${USER} 

#
# compilation tools
#
# CC		= /usr/bin/g++
CC		= gcc
CXX		= g++
AR		= ar
RANLIB		= ranlib
MAKEDEPEND	= makedepend
PURIFY          = purify
ETAGS		= etags
TOUCH		= touch

#
# object and binary files to build
#

ifdef OS
   ifdef THREADS
      OBJDIR	= ${OS}/
   else
      OBJDIR	= ${OS}.st/
   endif
endif

LIBOBJS		= ${LIBSRCS:%.cc=%.o}
BINOBJS		= ${BINSRCS:%.cc=%.o}
OBJS		= ${LIBOBJS} ${BINOBJS}
BINS		= ${BINOBJS:%.o=%}
BINS.PURIFY	= ${BINOBJS:%.o=%.purify}

REALLIBOBJS	= ${LIBOBJS:%=${OBJDIR}%}
REALBINOBJS	= ${BINOBJS:%=${OBJDIR}%}
REALLIBRARY	= ${OBJDIR}${LIBRARY}
REALOBJS	= ${REALLIBOBJS} ${REALBINOBJS}
REALBINS	= ${BINS:%=${OBJDIR}%}
REALBINS.PURIFY	= ${BINS.PURIFY:%=${OBJDIR}%}

#
# target dependency rules
#

all:        ${BINS} ${LIBRARY}
all.purify: ${BINS.PURIFY} ${LIBRARY}
library lib:    ${LIBRARY}

${BINS}: %: ${OBJDIR}%
	ln -fs $< $@

${BINS.PURIFY}:	%: ${OBJDIR}%
	ln -fs $< $@

${LIBRARY}:	${REALLIBRARY}

${OBJS}: %: ${OBJDIR}%


${REALLIBRARY}: ${REALLIBOBJS}
		${AR} cr ${REALLIBRARY} $?
		${RANLIB} ${REALLIBRARY}

${REALOBJS}: ${OBJDIR}%.o: %.cc
	${CXX} ${CCFLAGS} -o $@ -c $<

${REALBINS}: %: %.o ${REALLIBRARY}
	${CXX} ${CCFLAGS} -o $@  $< ${REALLIBRARY} ${LIBS} 

${REALBINS.PURIFY}: %.purify: %.o ${REALLIBRARY}
	${PURIFY} ${PFLAGS} ${CC} ${CCFLAGS} -o $@  $< ${REALLIBRARY} ${LIBS} 

clean:
	@echo "Removing objects..."
	rm -f ${REALBINS} ${REALBINS.PURIFY} ${REALLIBRARY} ${REALOBJS} core 
#	@echo "Removing purify cache files..."
#	pure_remove_old_files . 0

touch:
	${TOUCH} ${REALLIBOBJS} ${REALLIBRARY}

tags TAGS:
	${ETAGS} ${SRCS}

depend: ${SRCS}
	${MAKEDEPEND} -p${OBJDIR} ${INCS} ${SRCS}

# DO NOT DELETE


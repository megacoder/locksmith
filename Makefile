# vim: ts=8 sw=8

PREFIX	:=${HOME}/opt/$(shell uname -m)
BINDIR	=${PREFIX}/bin

CC	=ccache gcc -march=i686
DEFS	:=$(shell getconf LFS_CFLAGS) -D_XOPEN_SOURCE=500
CFLAGS	=-pipe -Os -Wall -Werror -g ${DEFS}
LDFLAGS	:=-g $(shell getconf LFS_LDFLAGS)
LDFLAGS	=

CFILES	:=$(wildcard *.c)
HFILES	:=$(wildcard *.h)

ARGS	=-v

all::	locksmith

clean::
	[ -d junk ] && chmod -R-R  +w junk && ${RM} -r junk
	${RM} *.o a.out core.* lint tags

distclean clobber:: clean
	${RM} locksmith

check::	locksmith junk
	./locksmith -r lock ${ARGS} junk

junk::
	${RM} -r junk
	mkdir -m 777 -p junk/first/second/third/fourth/fifth
	touch      junk/file junk/first/second/third/fourth/fifth/HOWZIT
	chmod 0666 junk/file junk/first/second/third/fourth/fifth/HOWZIT

check-lock:: locksmith
	./locksmith -r lock ${ARGS} junk

check-unlock:: locksmith
	./locksmith -r unlock ${ARGS} junk

tags::	${HFILES} ${CFILES}
	ctags ${HFILES} ${CFILES}

install:: locksmith
	install -d ${BINDIR}
	install -c -s locksmith ${BINDIR}
	cd ${BINDIR} && ln -sf ./locksmith lock
	cd ${BINDIR} && ln -sf ./locksmith unlock

uninstall::
	cd ${BINDIR} && ${RM} locksmith lock unlock

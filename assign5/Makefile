# Make x86-64 random byte generators.
# Copyright 2015, 2020, 2021 Paul Eggert

# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

# Optimization level.  Change this -O2 to -Og or -O0 or whatever.
OPTIMIZE = -Og

# The C compiler and its options.
# CC = gcc
# CFLAGS = $(OPTIMIZE) -g3 -Wall -Wextra \
#   -march=native -mtune=native -mrdrnd

CC ?= cc

BASE_CFLAGS = $(OPTIMIZE) -g3 -Wall -Wextra -march=native -mtune=native
IS_CLANG := $(shell $(CC) --version 2>&1 | grep -c clang)
IS_X86   := $(shell $(CC) -dumpmachine 2>/dev/null | grep -c x86_64)

SRCS := randall.c options.c output.c rand64-sw.c rand64-file.c
ifeq ($(IS_X86),1)
  SRCS += rand64-hw.c                # only compile on x86
endif

ifeq ($(IS_CLANG),0)   # GCC
  BASE_CFLAGS += -fanalyzer
endif
ifeq ($(IS_X86),1)     # x86-64 only
  BASE_CFLAGS += -mrdrnd
endif
CFLAGS = $(BASE_CFLAGS)

# The archiver command, its options and filename extension.
TAR = tar
TARFLAGS = --gzip --transform 's,^,randall/,'
TAREXT = tgz

#SRCS := randall.c options.c output.c rand64-hw.c rand64-sw.c
OBJS := $(SRCS:.c=.o)
TARGET := randall

# default: randall

# randall: randall.c
# 	$(CC) $(CFLAGS) $@.c -o $@

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

assignment: randall-assignment.$(TAREXT)
assignment-files = COPYING Makefile randall.c
randall-assignment.$(TAREXT): $(assignment-files)
	$(TAR) $(TARFLAGS) -cf $@ $(assignment-files)

submission-tarball: randall-submission.$(TAREXT)
submission-files = $(SRCS) $(SRCS:.c=.h) Makefile COPYING notes.txt tests test-llm.txt

randall-submission.$(TAREXT): $(submission-files)
	$(TAR) $(TARFLAGS) -cf $@ $(submission-files)

repository-tarball:
	$(TAR) -czf randall-git.tgz .git

.PHONY: default clean assignment submission-tarball repository-tarball

clean:
	$(RM) $(OBJS) $(TARGET) *.$(TAREXT)

# ----- BEGIN MAKEFILE FRAGMENT -----

RANDALL = ./randall
PYTHON = python3
TMP = /tmp/randall-test-$$$$

.PHONY: check clean-test

check: clean-test
	@echo "Running randall test suite..."
	@trap 'rm -rf $(TMP)' EXIT; \
	mkdir -p $(TMP); \
	tmp=$(TMP); \
	passed=0; failed=0; \
	for test in T01 T02 T03 T04 T05 T06 T07 T08 T09 T10 T11 T12 T13 T14 T15 T16 T17 T18; do \
		echo "Running $$test..."; \
		if $(MAKE) -s $$test TMP=$$tmp; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
			echo "FAIL: $$test"; \
		fi; \
	done; \
	echo "Results: $$passed passed, $$failed failed"; \
	test $$failed -eq 0

T01:
	@tmp=$(TMP); \
	$(RANDALL) 100 > $$tmp/out 2>$$tmp/err; \
	test $$? -eq 0 && test $$(wc -c < $$tmp/out) -eq 100

T02:
	@tmp=$(TMP); \
	$(RANDALL) 0 > $$tmp/out 2>$$tmp/err; \
	test $$? -eq 0 && test $$(wc -c < $$tmp/out) -eq 0

T03:
	@tmp=$(TMP); \
	$(RANDALL) -i /dev/urandom 50 > $$tmp/out 2>$$tmp/err; \
	test $$? -eq 0 && test $$(wc -c < $$tmp/out) -eq 50

T04:
	@tmp=$(TMP); \
	$(RANDALL) -o stdio 75 > $$tmp/out 2>$$tmp/err; \
	test $$? -eq 0 && test $$(wc -c < $$tmp/out) -eq 75

T05:
	@tmp=$(TMP); \
	$(RANDALL) -i /dev/urandom -o 64 256 > $$tmp/out 2>$$tmp/err; \
	test $$? -eq 0 && test $$(wc -c < $$tmp/out) -eq 256

T06:
	@tmp=$(TMP); \
	$(RANDALL) -1 100 > $$tmp/out 2>$$tmp/err; \
	test $$? -ne 0

T07:
	@tmp=$(TMP); \
	$(RANDALL) > $$tmp/out 2>$$tmp/err; \
	test $$? -ne 0

T08:
	@tmp=$(TMP); \
	$(RANDALL) -i /nonexistent/file 100 > $$tmp/out 2>$$tmp/err; \
	test $$? -ne 0

T09:
	@tmp=$(TMP); $(RANDALL) 1048576 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); sys.exit(1 if len(data)!=1048576 or data==b'\\x00'*len(data) else 0)"

T10:
	@tmp=$(TMP); $(RANDALL) 1024 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); chunk=data[:8] if len(data)>=16 else b''; sys.exit(1 if len(data)>=16 and all(data[i:i+8]==chunk for i in range(8,min(len(data),64),8)) else 0)"

T11:
	@tmp=$(TMP); $(RANDALL) 1024 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); sys.exit(1 if len(data)>=256 and all(data[i]==i%256 for i in range(min(256,len(data)))) else 0)"

T12:
	@tmp=$(TMP); $(RANDALL) 8192 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); values=list(data[:1000]) if len(data)>=1000 else []; [sys.exit(1) for p in range(2,257) if p*3<len(values) and all(values[i]==values[i%p] for i in range(p,min(p*3,len(values))))]"

T13:
	@tmp=$(TMP); $(RANDALL) 1024 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); sys.exit(1 if len(data)>=16 and all(data[i:i+8]==data[i+8:i+16][::-1] for i in range(0,len(data)-15,16)) else 0)"

T14:
	@tmp=$(TMP); $(RANDALL) 1048576 > $$tmp/data; $(PYTHON) -c "import sys,math; data=open('$$tmp/data','rb').read(); n=len(data)*8; ones=sum(bin(b).count('1') for b in data); z=abs(ones-n//2)/math.sqrt(n); print('Z-score:',z); sys.exit(1 if z>=4.5 else 0)"

T15:
	@tmp=$(TMP); $(RANDALL) 1048576 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); obs=[0]*256; [obs.__setitem__(b,obs[b]+1) for b in data]; exp=len(data)/256; chi2=sum((o-exp)**2/exp for o in obs); print('Chi2:',chi2); sys.exit(1 if chi2>310.46 else 0)"

T16:
	@tmp=$(TMP); $(RANDALL) 1048576 > $$tmp/data; $(PYTHON) -c "import sys,math; data=open('$$tmp/data','rb').read(); n=len(data)-1; sx=sum(data[:-1]); sy=sum(data[1:]); sxy=sum(data[i]*data[i+1] for i in range(n)); sx2=sum(x*x for x in data[:-1]); sy2=sum(y*y for y in data[1:]); dx=n*sx2-sx*sx; dy=n*sy2-sy*sy; r=abs(n*sxy-sx*sy)/math.sqrt(dx*dy) if dx>0 and dy>0 else 0; print('Corr:',r); sys.exit(1 if r>0.1 else 0)"

T17:
	@tmp=$(TMP); $(RANDALL) 2048576 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read(); n=len(data)*8; ones=sum(bin(b).count('1') for b in data); z=abs(ones-n//2)/(n**0.5); sys.exit(1 if z>=4.5 else 0)"

T18:
	@tmp=$(TMP); $(RANDALL) 512 > $$tmp/data; $(PYTHON) -c "import sys; data=open('$$tmp/data','rb').read()[:100]; pat={}; [pat.__setitem__(n,pat.get(n,0)+1) for b in data for n in [(b>>4)&15,b&15]]; exp=200/16; chi2=sum((c-exp)**2/exp for c in pat.values()); sys.exit(1 if chi2>50 else 0)"

clean-test:
	@rm -rf $(TMP) 2>/dev/null || true

# ----- END MAKEFILE FRAGMENT -----
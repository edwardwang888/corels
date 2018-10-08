CFLAGS := -g -W -Wall -Wno-unused-parameter -fPIC -I. -DGMP -O2 -fno-omit-frame-pointer -I/usr/local/include #-stdlib=libc++

ifneq (,$(filter $(shell uname -n),elaines-MacBook-Pro.local Nicholass-Air-5.wireless.yale.internal))
    LIBS := -L/usr/local/lib
    CFLAGS += -I/usr/local/include
else
    LIBS :=
endif

LIBS += -L/usr/local/lib -lgmpxx -lgmp

ifneq ($(shell uname -n),beepboop)
    LDFLAGS :=
    CFLAGS += -fno-omit-frame-pointer
    ifneq ($(shell uname -r),15.6.0)
        #LIBS += -lprofiler -ltcmalloc #Didn't install these yet
    endif
else
    LDFLAGS :=
endif

ifneq ($(shell uname),Darwin)
    #LIBS += -lbsd
endif

CC = gcc
CXX = g++ -std=c++11
#CC = clang
#CXX = clang++ -std=c++11

DEPSDIR := .deps

CLEAN = corels corels-cli evaluate-test test *~ *.o *.so *.a

all: corels #libcorels.so evaluate-test

corels: main.o corels.o pmap.o cache.o rulelib.o utils.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

corels-cli: cli.o params.o run.o corels.o pmap.o cache.o rulelib.o utils.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

libcorels.so: run.o params.o corels.o pmap.o cache.o rulelib.o utils.o evaluate.o
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(LIBS) $(CFLAGS)

libcorels.a: run.o params.o corels.o pmap.o cache.o rulelib.o utils.o
	ar rcs $@ $^

evaluate-test: evaluate-test.o evaluate.o params.o run.o corels.o pmap.o cache.o rulelib.o utils.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

main.o: main.cc wpa_objective.h $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) $(DEPCFLAGS) -c $< -o $@

corels.o: corels.cc $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) $(DEPCFLAGS) -c $< -o $@

pmap.o: pmap.cc $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) $(DEPCFLAGS) -c $< -o $@

utils.o: utils.cc $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) $(DEPCFLAGS) -c $< -o $@

cache.o: cache.cc $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) $(DEPCFLAGS) -c $< -o $@

rulelib.o: rulelib.c $(DEPSDIR)/stamp
	$(CC) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

evaluate-test.o: evaluate-test.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

evaluate.o: evaluate.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

$(DEPSDIR)/stamp:
	mkdir -p $(dir $@)
	touch $@

clean:
	$(RM) $(wildcard $(CLEAN))
	$(RM) -r $(DEPSDIR)

test: test.c
	gcc -Wall -Wextra -o test test.c

corels_test: corels_test.c rulelib.c wpa_objective.h
	gcc -L/usr/local/lib -DGMP -o corels_test corels_test.c rulelib.c -lgmpxx -lgmp 

baseline: baseline.c wpa_objective.h rulelib.c
	gcc -L/usr/local/lib -DGMP -o baseline baseline.c rulelib.c -lgmpxx -lgmp 

DEPFILES := $(wildcard $(DEPSDIR)/*.d)
ifneq ($(DEPFILES),)
include $(DEPFILES)
endif

.PHONY: all clean
.SUFFIXES:

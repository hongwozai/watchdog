TARGET = $(notdir $(shell pwd))

SUBDIR = .

# unittest
TESTDIR    = test
TESTTARGET = $(addsuffix Test, $(TARGET))

CC = gcc

CFLAGS  = -g3 -Wall -Iinc
LDFLAGS =

SRCS  = $(wildcard $(addsuffix /*.cpp, $(SUBDIR)))
SRCS += $(wildcard $(addsuffix /*.c,   $(SUBDIR)))
DEPS := $(patsubst %.c, %.deps, $(patsubst %.cpp, %.deps, $(SRCS)))
OBJS := $(patsubst %.c, %.o,    $(patsubst %.cpp, %.o,    $(SRCS)))

# unittest
TESTSRCS := $(SRCS)
TESTSRCS := $(filter-out %/Main.cpp, $(TESTSRCS))
TESTSRCS := $(filter-out %/Main.c,   $(TESTSRCS))
TESTSRCS += $(wildcard $(addsuffix /*.cpp, $(TESTDIR)))
TESTSRCS += $(wildcard $(addsuffix /*.c,   $(TESTDIR)))
TESTDEPS := $(patsubst %.c, %.deps, $(patsubst %.cpp, %.deps, $(TESTSRCS)))
TESTOBJS := $(patsubst %.c, %.o,    $(patsubst %.cpp, %.o,    $(TESTSRCS)))

ifeq ("$(SRCS)", " ")
exit:
endif

# fake target
.PHONY: all run clean coverage release debug echo test exit

all: $(TARGET)

run: $(TARGET)
	./$(TARGET) -a /bin/sh -e "--aa" -e "aa" -e "bb"

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJS)
	rm -rf $(DEPS)

test: CFLAGS += -UNDEBUG
test: LDFLAGS += -lgtest -lpthread
test: $(TESTTARGET)
	./$(TESTTARGET)

coverage: CFLAGS  += --coverage
coverage: LDFLAGS += --coverage
coverage: all

release: CFLAGS += -O2
release: all

ifneq ("$(MAKECMDGOALS)", "clean")
include $(DEPS)
endif
ifeq ("$(MAKECMDGOALS)", "test")
include $(TESTDEPS)
endif

# real target
echo:
	@echo $(SRCS)
	@echo $(DEPS)
	@echo $(OBJS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TESTTARGET): $(TESTOBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.deps: %.cpp
	@set -e ; \
	$(CC) $(CFLAGS) -MM $< > $@; \
	sed -r -i "s,(.*):,$(dir $@)\1:,g" $@;

%.deps: %.c
	@set -e ; \
	$(CC) $(CFLAGS) -MM $< > $@; \
	sed -r -i "s,(.*):,$(dir $@)\1:,g" $@;

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

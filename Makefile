# makefile parameters
SRCDIR      := src
LIBSDIR     :=
TESTDIR     := tests
BUILDDIR    := int
TARGETDIR   := target
SRCEXT      := c

# compiler parameters
CC          := gcc
CFLAGS      := -std=c99 -Wall -Wpedantic -Werror -Wno-unused-function
LIB         := z
INC         := /usr/local/include
DEFINES     :=


#---------------------------------------------------------------------------------
# DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------

ifneq ($(DEBUG),)
	CFLAGS += -ggdb
else
	CFLAGS += -O3
endif

# sets the src directory in the VPATH
VPATH := $(SRCDIR)

# source files
SRCS = $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
ifneq ($(LIBSDIR),)
	SRCS += $(shell find $(LIBSDIR) -type f -name *.$(SRCEXT))
endif
TEST_SRCS = $(shell find $(TESTDIR) -type f -name *.$(SRCEXT))
# object files
OBJS = $(patsubst %,$(BUILDDIR)/a/%,$(SRCS:.$(SRCEXT)=.o))

TEST_OBJS = $(patsubst %,$(BUILDDIR)/tests/%,$(TEST_SRCS:.$(SRCEXT)=.o))
TEST_OBJS += $(patsubst %,$(BUILDDIR)/tests/%,$(SRCS:.$(SRCEXT)=.o))

# includes the flag to generate the dependency files when compiling
CFLAGS += -MD

# adds the include prefix to the include directories
INC := $(addprefix -I,$(INC)) -I$(SRCDIR) -I$(LIBSDIR)

# adds the lib prefix to the libraries
LIB := $(addprefix -l,$(LIB))

# adds the define prefix to the defines
DEFINES := $(addprefix -D,$(DEFINES))


# default: compiles all the binaries
all: tests

# compiles and runs the unit tests
tests: $(TARGETDIR)/tests
	./$(TARGETDIR)/tests

# shows usage
help:
	@echo "To compile and run the tests:"
	@echo
	@echo "\t\033[1;92m$$ make tests\033[0m"
	@echo
	@echo "Compiled binaries can be found in \033[1;92m$(TARGETDIR)\033[0m."
	@echo
	@echo "\033[1;92mmake format\033[0m runs clang-format on every source and header file."
	@echo

# clean objects and binaries
clean:
	@$(RM) -rf $(BUILDDIR) $(TARGETDIR)

# creates the directories
dirs:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

# runs clang-format in all the project source files
format:
	@find libs/ -iname *.h -o -iname *.$(SRCEXT) | xargs clang-format -i -style=file
	@find src/ -iname *.h -o -iname *.$(SRCEXT) | xargs clang-format -i -style=file

# INTERNAL: builds the test binary
$(TARGETDIR)/tests: $(TEST_OBJS) | dirs
	@$(CC) $(CFLAGS) $(INC) $(DEFINES) $^ $(LIB) -o $@
	@echo "LD $@"

# rule to build test object files
$(BUILDDIR)/tests/%.o: %.$(SRCEXT)
	@mkdir -p $(basename $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(INC) -Itests $(DEFINES) -c -o $@ $<

# rule to build lib object files
$(BUILDDIR)/a/%.o: %.$(SRCEXT)
	@mkdir -p $(basename $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(INC) $(DEFINES) -c -o $@ $<

.PHONY: clean dirs tests all tool

# includes generated dependency files
-include $(OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)

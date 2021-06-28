# SHELL SECTION
RM := rm -rf

# PROJECT TREE
SDIR := ./src
IDIR := ./inc
ADIR := ./app
TDIR := ./test

# FILES
SRC := $(wildcard $(SDIR)/*.c)

ASRC := $(SRC) $(wildcard $(ADIR)/*.c)
TSRC := $(SRC) $(wildcard $(TDIR)/*.c)

AOBJ := $(ASRC:%.c=%.o)
TOBJ := $(TSRC:%.c=%.o)

OBJ := $(AOBJ) $(TOBJ)

DEPS := $(OBJ:%.o=%.d)

# EXEC
EXEC := main.out
TEXEC := test.out

# COMPILATOR SECTION

# By default use gcc
CC ?= gcc

C_FLAGS := -Wall -Wextra -Winline -pthread

DEP_FLAGS := -MMD -MP

H_INC := $(foreach d, $(IDIR), -I$d)

ifeq ($(CC),clang)
	C_FLAGS += -Weverything
else ifneq (, $(filter $(CC), cc gcc))
	C_FLAGS += -rdynamic
endif

ifeq ("$(origin O)", "command line")
	OPT := -O$(O)
else
	OPT := -O3
endif

ifeq ("$(origin G)", "command line")
	GGDB := -ggdb$(G)
else
	GGDB :=
endif

C_FLAGS += $(OPT) $(GGDB) $(DEP_FLAGS)

all: $(EXEC)

test: $(TEXEC)

$(EXEC): $(AOBJ)
	$(CC) $(C_FLAGS) $(H_INC) $(AOBJ) -o $@

$(TEXEC): $(TOBJ)
	$(CC) $(C_FLAGS) $(H_INC) $(TOBJ) -o $@

%.o:%.c %.d
	$(CC) $(C_FLAGS) $(H_INC) -c $< -o $@

clean:
	$(RM) $(EXEC)
	$(RM) $(TEXEC)
	$(RM) $(OBJ)
	$(RM) $(DEPS)

$(DEPS):

include $(wildcard $(DEPS))
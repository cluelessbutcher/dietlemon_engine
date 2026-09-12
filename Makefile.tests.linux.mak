BUILD_DIR := bin
OBJ_DIR := obj

ASSEMBLY := tests
EXTENSION := 
COMPILER_FLAGS := -g -MD -Werror=vla -fdeclspec -fPIC
INCLUDE_FLAGS := -Iengine/src -I$(VULKAN_SDK)\include
LINKER_FLAGS := -L./$(BUILD_DIR)/ -lengine -Wl,-rpath,.
DEFINES := -D_DEBUG -DKIMPORT

# Make does not offer a recursive wildcard function, so here's one:
#rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES := $(shell find $(ASSEMBLY) -name *.c)
DIRECTORIES := $(shell find $(ASSEMBLY) -type d)
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)

all: scaffold compile link

.PHONY: scaffold
scaffold:
	@echo Scaffolding folder structure...
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES)
	@echo Linking $(ASSEMBLY)...
	clang $(OBJ_FILES) -o $(BUILD_DIR)/$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)

.PHONY: compile
compile:
	@echo Compiling...

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/$(ASSEMBLY)
	rm -rf $(OBJ_DIR)/$(ASSEMBLY)

$(OBJ_DIR)/%.c.o: %.c 
	@echo   $<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

-include $(OBJ_FILES:.o=.d)
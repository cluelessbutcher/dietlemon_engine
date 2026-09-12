BUILD_DIR := bin
OBJ_DIR := obj

ASSEMBLY := testbed
EXTENSION := 
COMPILER_FLAGS := -g -MD -fdeclspec -fPIC
INCLUDE_FLAGS := -Iengine/src -Itestbed\src
LINKER_FLAGS := -L./$(BUILD_DIR)/ -lengine -Wl,-rpath,.
DEFINES := -D_DEBUG -DDIMPORT

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
	rm -rf $(BUILD_DIR)\$(ASSEMBLY)
	rm -rf $(OBJ_DIR)\$(ASSEMBLY)

$(OBJ_DIR)/%.c.o: %.c
	@echo   $<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)
	
-include $(OBJ_FILES:.o=.d)
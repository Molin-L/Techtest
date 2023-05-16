# Makefile
PROJECT_DIR := $(CURDIR)
# Define the commands
CLEAN_CMD := rm -rf $(CURDIR)/build
BUILD_CMD := cmake --build $(CURDIR)/build --config Debug --target all -j 12 --
CONFIGURE_CMD := cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -S$(CURDIR) -B$(CURDIR)/build -G "Unix Makefiles"
REBUILD_CMD := $(CLEAN_CMD) && $(CONFIGURE_CMD) && $(BUILD_CMD)

# Define the targets
.PHONY: clean build rebuild

# Target: clean
clean:
	$(CLEAN_CMD)

# Target: build
build:
	$(BUILD_CMD)

# Target: rebuild
rebuild:
	$(REBUILD_CMD)

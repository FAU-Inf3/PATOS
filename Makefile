#----------------------------------------------------------------------------
# Makefile for building the Patos source-to-source compiler
#
# Patrick Kreutzer (patrick.kreutzer@fau.de)
#----------------------------------------------------------------------------

# Run 'make VERBOSE=1' to output executed commands
ifdef VERBOSE
	VERB := 
else
	VERB := @
endif


# Set C++ compiler and compiler flags used to compile the Patos compiler
CXX := g++
# We have to explicitly enable exceptions again, because boost needs (wants) exceptions, whereas llvm-config disables them...
CXXFLAGS := -fexceptions -g


# Set linker and linker flags used to link the Patos compiler's object files
LD := g++
LDFLAGS :=


# Path to the llvm-config tool
# If this tool resides in a global path, 'llvm-config' should suffice
LLVM_CONFIG := llvm-config-3.5


# Compiler and linker flags needed to link against the llvm/clang library
LLVM_CXXFLAGS := `$(LLVM_CONFIG) --cxxflags`
LLVM_LDFLAGS := `$(LLVM_CONFIG) --ldflags --libs --system-libs`


# Additional include path to find llvm/clang headers
LLVM_INCLUDE := -I`$(LLVM_CONFIG) --includedir`


# Clang libraries to link against
# The use of '--start-group ... --end-group' makes it easier to maintain the list of
#   libraries, since there might be circular dependencies. Note,
#   however, that this option might have a significant performance cost.
CLANG_LIBS := \
	-Wl,--start-group \
		-lclangAST \
		-lclangAnalysis \
		-lclangBasic \
		-lclangDriver \
		-lclangEdit \
		-lclangFrontend \
		-lclangFrontendTool \
		-lclangLex \
		-lclangParse \
		-lclangSema \
		-lclangEdit \
		-lclangASTMatchers \
		-lclangRewrite \
		-lclangRewriteFrontend \
		-lclangStaticAnalyzerFrontend \
		-lclangStaticAnalyzerCheckers \
		-lclangStaticAnalyzerCore \
		-lclangSerialization \
		-lclangTooling \
	-Wl,--end-group


# Boost libraries to link against
BOOST_LIBS := \
	-L /usr/lib \
	-lboost_system \
	-lboost_filesystem \
	-lboost_program_options \


# Directories for the source and object files, as well as for the dependency files
DIR_SRC := src
DIR_OBJ := obj
DIR_DEP := dep

# Target file (executable)
DIR_BIN := bin
TARGET := $(DIR_BIN)/patos

# Get all source and object files
SRC_FILES := $(shell find $(DIR_SRC) -name "*.cpp")
OBJ_FILES := $(patsubst %.cpp, $(DIR_OBJ)/%.o, $(notdir $(SRC_FILES)))


#----------------------------------------------------------------------------
#----------------------------------------------------------------------------


# Print 'header' whenever make is issued
$(info -------------------------------)
$(info Patos source-to-source compiler)
$(info -------------------------------)
$(info )


#----------------------------------------------------------------------------
#----------------------------------------------------------------------------


.PHONY: all
all: $(TARGET)


$(TARGET): $(OBJ_FILES)
	$(VERB)mkdir -p $(DIR_BIN)
	@echo -e 'LD\t$@'
	$(VERB)$(CXX) $(LDFLAGS) $^ $(BOOST_LIBS) $(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@


-include $(patsubst %.cpp, $(DIR_DEP)/%.md, $(notdir $(SRC_FILES)))


$(DIR_OBJ)/%.o: $(DIR_SRC)/%.cpp
	$(VERB)mkdir -p $(DIR_OBJ)
	$(VERB)mkdir -p $(DIR_DEP)
	@echo -e 'DEP\t$<'
	$(VERB)$(CXX) $(LLVM_CXXFLAGS) $(CXXFLAGS) $(LLVM_INCLUDE) -MM -MP -MT $(DIR_OBJ)/$(*F).o -MT $(DIR_DEP)/$(*F).md $< > $(DIR_DEP)/$(*F).md
	@echo -e 'CMPL\t$<'
	$(VERB)$(CXX) $(LLVM_CXXFLAGS) $(CXXFLAGS) $(LLVM_INCLUDE) -c $< -o $@


.PHONY: clean
clean:
	@echo -e 'RM\t $(DIR_OBJ)'
	$(VERB)rm -rf $(DIR_OBJ)
	@echo -e 'RM\t $(DIR_BIN)'
	$(VERB)rm -rf $(DIR_BIN)
	@echo -e 'RM\t $(DIR_DEP)'
	$(VERB)rm -rf $(DIR_DEP)

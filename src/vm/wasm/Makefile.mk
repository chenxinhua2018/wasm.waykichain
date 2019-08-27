
OPT = -DNDEBUG -DEOS_VM_USE_EXTERNAL_OUTCOME

PLATFORM_LDFLAGS=-pthread
PLATFORM_CCFLAGS= -fno-builtin-memcmp -pthread -DOS_LINUX
PLATFORM_CXXFLAGS= -fno-builtin-memcmp -pthread -DOS_LINUX

# include softfloat.mk


WASM_TYPES = types
EOSVM_INCLUDE_DIR = eos-vm/include
EOSVM_OUTCOME_DIR = eos-vm/external/outcome/single-header
EOSVM_SOFTFLOAT_INCLUDE = eos-vm/external/softfloat/source/include
EOSVM_COMPILER_BUILTINS = eos-vm/external/compiler_builtins


EOSVM_SRC_HPPS = \
$(EOSVM_INCLUDE_DIR)/eosio/vm/allocator.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/backend.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/base_visitor.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/config.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/constants.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/dispatcher.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/error_codes.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/error_codes_def.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/error_codes_pp.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/exceptions.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/execution_context.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/guarded_ptr.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/host_function.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/interpret_visitor.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/leb128.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/opcodes.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/opcodes_def.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/parser.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/sections.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/softfloat.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/stack_elem.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/types.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/utils.hpp \
# ${EOSVM_INCLUDE_DIR}/eosio/vm/validation.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/vector.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/wasm_stack.hpp \
$(EOSVM_INCLUDE_DIR)/eosio/vm/watchdog.hpp 


COMPILER_BUILTINS_SRC = \
$(EOSVM_COMPILER_BUILTINS)/fixdfti.c \
$(EOSVM_COMPILER_BUILTINS)/fixsfti.c \
$(EOSVM_COMPILER_BUILTINS)/fixtfti.c \
$(EOSVM_COMPILER_BUILTINS)/fixunsdfti.c \
$(EOSVM_COMPILER_BUILTINS)/fixunssfti.c \
$(EOSVM_COMPILER_BUILTINS)/fixunstfti.c \
$(EOSVM_COMPILER_BUILTINS)/floattidf.c \
$(EOSVM_COMPILER_BUILTINS)/floatuntidf.c 

CXXFLAGS += -std=c++17 -fPIC ${PLATFORM_CXXFLAGS} -I. -I$(EOSVM_INCLUDE_DIR) \
                                                      -I${EOSVM_OUTCOME_DIR} \
                                                      -I${EOSVM_SOFTFLOAT_INCLUDE} \
                                                      -I${EOSVM_COMPILER_BUILTINS} \
                                                      -I${WASM_TYPES}

LDFLAGS += $(PLATFORM_LDFLAGS)

WASM_EMPTY =
WASM_PATH_SOURCE = $(EOSVM_COMPILER_BUILTINS)/
WASM_OBJECTS = 	$(subst $(WASM_PATH_SOURCE), $(WASM_EMPTY), $(COMPILER_BUILTINS_SRC:.c=.o))


# libwasm.a: ${OBJECTS} wasminterface.o 
libwasm.a:
	@echo "building libwasm"
	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} ${OPT} $(COMPILER_BUILTINS_SRC)
	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} ${OPT} wasminterface.cpp $(WASM_TYPES)/uint128.cpp 
	@$(AR) rcs $@ ${WASM_OBJECTS} wasminterface.o uint128.o 
	@rm -rf *.o

# uint128.o: 
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(WASM_TYPES)/uint128.cpp

# fixdfti.o:
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/fixdfti.c

# fixsfti.o:
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/fixsfti.c

# fixtfti.o:
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/fixtfti.c

# fixunsdfti.o:
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/fixunsdfti.c

# fixunssfti.o:		
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/fixunssfti.c

# fixunstfti.o:	
#  	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/fixunstfti.c

# floattidf.o:	
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/floattidf.c

# floatuntidf.o:	
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} $(OPT) $(EOSVM_COMPILER_BUILTINS)/floatuntidf.c

# wasminterface.o : wasminterface.cpp wasminterface.h wasmcontext.hpp wasmhostmethods.hpp ${EOSVM_SRC_HPPS}
# 	@echo "building wasminterface.o"
# 	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} ${OPT} wasminterface.cpp

clean:
	rm -rf *.o *.a

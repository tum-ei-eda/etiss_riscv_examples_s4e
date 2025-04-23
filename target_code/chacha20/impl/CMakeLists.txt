set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${RISCV_TOOLCHAIN_PREFIX}/riscv32-unknown-elf/include -Ofast")

if( CHACHA_EXTENSION )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv32ixchacha")
  add_compile_definitions(CHACHA_EXTENSION)
endif()
add_library(chacha20_lib OBJECT main.c)

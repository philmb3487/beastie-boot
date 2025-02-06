cmake_minimum_required(VERSION 3.28)
project(beastie LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BEASTIE_SYSTEM_ASMJIT "Use system asmjit" OFF)
option(BEASTIE_STATIC "Enable static build" OFF)
option(BEASTIE_USE_LLVM "Enable llvm disassembler" OFF)

if(BEASTIE_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(BUILD_SHARED_LIBS OFF)
    set(ASMJIT_STATIC ON)
endif(BEASTIE_STATIC)

add_executable(beastie
    src/bootassembler.hxx src/bootassembler.cxx
    src/bootloader.hxx src/bootloader.cxx
    src/cenvironmentwriter.hxx src/cenvironmentwriter.cxx
    src/cmetawriter.hxx src/cmetawriter.cxx
    src/constants.hxx
    src/csymbolswriter.hxx src/csymbolswriter.cxx
    src/main.cxx
    src/misc.hxx src/misc.cxx
    src/types.hxx
    src/cvmwaregfx.hxx src/cvmwaregfx.cxx
    src/ci915gfx.hxx src/ci915gfx.cxx
    src/cgfx.hxx
)
target_compile_options(beastie PRIVATE -Wall)

target_include_directories(beastie PUBLIC deps/zlib/)
target_sources(beastie PUBLIC
    deps/zlib/adler32.c
    deps/zlib/compress.c
    deps/zlib/crc32.c
    deps/zlib/deflate.c
    deps/zlib/inffast.c
    deps/zlib/inflate.c
    deps/zlib/inftrees.c
    deps/zlib/trees.c
    deps/zlib/zutil.c
)

target_include_directories(beastie PUBLIC deps/iostreams/include/)
target_sources(beastie PUBLIC
    deps/iostreams/src/file_descriptor.cpp
    deps/iostreams/src/mapped_file.cpp
    deps/iostreams/src/zlib.cpp
    deps/iostreams/src/gzip.cpp
)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(BEASTIE_SYSTEM_ASMJIT)
    find_package(asmjit REQUIRED)
    target_include_directories(beastie SYSTEM PUBLIC asmjit::asmjit)
    target_link_libraries(beastie asmjit::asmjit)
else()
    FetchContent_Declare(
        asmjit
        GIT_REPOSITORY  https://github.com/asmjit/asmjit
        GIT_TAG         cfc9f81
    )
    set(ASMJIT_STATIC ON)
    FetchContent_MakeAvailable(asmjit)
    add_dependencies(beastie asmjit)
    target_include_directories(beastie SYSTEM PUBLIC asmjit::asmjit)
    target_link_libraries(beastie asmjit::asmjit)
endif()

if(BEASTIE_USE_LLVM)
    find_package(LLVM CONFIG REQUIRED)

    include_directories(${LLVM_INCLUDE_DIRS})
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    add_definitions(${LLVM_DEFINITIONS_LIST})

    target_link_libraries(beastie LLVM)

    target_compile_definitions(beastie PUBLIC DISASSEMBLER)
    target_sources(beastie PUBLIC src/disassembler.h src/disassembler.cpp)
endif()

include(GNUInstallDirs)
install(TARGETS beastie
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

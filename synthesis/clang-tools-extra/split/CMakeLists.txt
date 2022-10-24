set(LLVM_LINK_COMPONENTS support)

add_clang_executable(split
  Split.cpp
  ../common/common.cpp
  ../common/type.cpp
)

ADD_DEFINITIONS(-fexceptions)
REMOVE_DEFINITIONS(-fno-exceptions)

target_link_libraries(split
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  clangToolingCore
  clangRewrite
  LLVMFrontendOpenMP
)

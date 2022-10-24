set(LLVM_LINK_COMPONENTS support)

add_clang_executable(synthesize
  synthesize.cpp
  ../common/common.cpp
  ../common/type.cpp
)

# 加了没用
ADD_DEFINITIONS(-fexceptions)
REMOVE_DEFINITIONS(-fno-exceptions)

target_link_libraries(synthesize
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

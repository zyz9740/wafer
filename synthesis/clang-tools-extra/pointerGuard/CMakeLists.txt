set(LLVM_LINK_COMPONENTS support)

add_clang_executable(pointerGuard
  PointerGuard.cpp
  ../common/common.cpp
  ../common/type.cpp
)

target_link_libraries(pointerGuard
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


set(LLVM_LINK_COMPONENTS support)

add_clang_executable(floatGuard
  FloatGuard.cpp
  ../common/common.cpp
  ../common/type.cpp
)

target_link_libraries(floatGuard
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



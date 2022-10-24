set(LLVM_LINK_COMPONENTS support)

add_clang_executable(emi
  emi.cpp
)

target_link_libraries(emi
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

set(LLVM_LINK_COMPONENTS support)

add_clang_executable(shiftGuard
  ShiftGuard.cpp
  ../common/common.cpp
  ../common/type.cpp
)

target_link_libraries(shiftGuard
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

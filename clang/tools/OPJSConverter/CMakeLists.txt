set(LLVM_LINK_COMPONENTS
  Support
  )

#对应的cpp文件都要加进来
add_clang_executable(OPJSConverter
  OPJSConverter.cpp 
)

# 将目标文件与库文件进行链接
target_link_libraries(OPJSConverter
	PRIVATE
    clangAST
    clangBasic
    clangDriver
    clangFormat
    clangLex
    clangParse
    clangSema
    clangFrontend
    clangTooling
    clangToolingCore
    clangRewrite
    clangRewriteFrontend
)

if(UNIX)
  set(CLANGXX_LINK_OR_COPY create_symlink)
else()
  set(CLANGXX_LINK_OR_COPY copy)
endif()
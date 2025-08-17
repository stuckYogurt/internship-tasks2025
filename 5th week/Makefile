IDIR = ~/llvm_install_dir

PASS_NAME = MyPass
TEST_SRC = ./llvm-basic-pass-main/tests/input.c

.PHONY: build pass
build: $(LIB_SRC)
	cmake -DLT_LLVM_INSTALL_DIR="$(IDIR)" ./llvm-basic-pass-main/build/
	make -C ./llvm-basic-pass-main/build
	$(IDIR)/bin/clang-22 -O0 -S -emit-llvm $(TEST_SRC) -o IRs/input.ll

pass:
	$(IDIR)/bin/opt -load-pass-plugin ./llvm-basic-pass-main/build/libMyPass.so -passes=$(PASS_NAME) -disable-output IRs/input.ll
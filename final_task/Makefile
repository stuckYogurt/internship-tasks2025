IDIR = ~/llvm_install_dir

PASS_NAME = LICMPass
TEST_SRC = ./llvm-basic-pass-main/tests/input.c

.PHONY: build pass
build: $(LIB_SRC)
	cmake -DLT_LLVM_INSTALL_DIR="$(IDIR)" -B ./llvm-basic-pass-main/build/ -S ./llvm-basic-pass-main/build/
	make -C ./llvm-basic-pass-main/build
	$(IDIR)/bin/clang-22 -O0 -S -emit-llvm $(TEST_SRC) -o IRs/input.ll

pass:
	$(IDIR)/bin/opt -load-pass-plugin ./llvm-basic-pass-main/build/lib$(PASS_NAME).so -passes=$(PASS_NAME) -disable-output IRs/input.ll -o IRs/output.ll
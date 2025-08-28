IDIR = ~/llvm_install_dir

PASS_NAME = LICMPass
TEST_SRC = ./llvm-basic-pass-main/tests/input.c

.PHONY: build pass
build: $(LIB_SRC)
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0" -DLT_LLVM_INSTALL_DIR="$(IDIR)" -B ./llvm-basic-pass-main/build/ -S ./llvm-basic-pass-main/build/
	make -C ./llvm-basic-pass-main/build
	$(IDIR)/bin/clang-22 -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(TEST_SRC) -o IRs/input.ll
	$(IDIR)/bin/opt -passes='strip,mem2reg' IRs/input.ll -S -o IRs/input.ll
	cp IRs/input.ll IRs/output.ll

pass:
	$(IDIR)/bin/opt -load-pass-plugin ./llvm-basic-pass-main/build/lib$(PASS_NAME).so -passes=$(PASS_NAME) IRs/output.ll -S -o IRs/output.ll
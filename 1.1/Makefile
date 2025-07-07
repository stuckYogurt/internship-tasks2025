CXX = g++
BUILD_DIR = bin

.PHONY: build clean test
build: main.cpp $(BUILD_DIR)
	$(CXX) -std=c++17 main.cpp -o $(BUILD_DIR)/main 

$(BUILD_DIR):
	mkdir -p $@
	touch $@



test: build
	python3 test.py

clean:
	rm -f main

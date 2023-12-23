CXX = ccache g++
OUTPUT_FILE=bin/pipejs

define INCLUDE
	v8/include/
endef

define INCLUDEUV
	libuv/include/
endef

define SRC
	pipejs/index.cc
endef

define DEP
	v8/libv8_monolith.a
	libuv/libuv.a
endef

export INCLUDE
export INCLUDEUV
export OUTPUT_FILE

export SRC
export DEP

build:
	mkdir -p bin
	$(CXX) $$SRC -I $$INCLUDE -I $$INCLUDEUV -std=c++17 -pthread -o $$OUTPUT_FILE $< -DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX $$DEP -Wl,--no-as-needed -ldl

run:
	./bin/pipejs ./test.js

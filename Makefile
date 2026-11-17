.PHONY: all clean run windows test

all:
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && $(MAKE)

test: all
	@cd build && ctest --output-on-failure

windows:
	@cmake -S . -B build-windows -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc"
	@cmake --build build-windows

clean:
	rm -rf build build-windows dist

run: all
	./build/caddin

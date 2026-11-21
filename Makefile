.PHONY: all clean run windows android apk test dist

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

android:
	@cmake -S . -B build-android \
		-DCMAKE_TOOLCHAIN_FILE=/home/carlos/android-tools/ndk/build/cmake/android.toolchain.cmake \
		-DANDROID_ABI=arm64-v8a \
		-DANDROID_PLATFORM=android-24 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCADDIN_HEADLESS=ON
	@cmake --build build-android --target caddin caddin_monitor
	@/home/carlos/android-tools/ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip \
		build-android/caddin build-android/caddin_monitor 2>/dev/null || true
	@echo "[ANDROID] Binários nativos ARM64 (Bionic) prontos em build-android/"

apk:
	@cmake -S . -B build-apk \
		-DCMAKE_TOOLCHAIN_FILE=/home/carlos/android-tools/ndk/build/cmake/android.toolchain.cmake \
		-DANDROID_ABI=arm64-v8a \
		-DANDROID_PLATFORM=android-24 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCADDIN_ANDROID_APP=ON
	@cmake --build build-apk --target caddin
	@/home/carlos/android-tools/ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip build-apk/libcaddin.so 2>/dev/null || true
	@mkdir -p android_apk/res/values android_apk/lib/arm64-v8a dist
	@cp build-apk/libcaddin.so android_apk/lib/arm64-v8a/libcaddin.so
	@rm -f android_apk/caddin-unaligned.apk dist/caddin-v1.4.0.apk
	@aapt package -f -M android_apk/AndroidManifest.xml -S android_apk/res \
		-I /home/carlos/android-tools/android-34.jar -F android_apk/caddin-unaligned.apk
	@cd android_apk && aapt add caddin-unaligned.apk lib/arm64-v8a/libcaddin.so
	@zipalign -f -p 4 android_apk/caddin-unaligned.apk dist/caddin-v1.4.0.apk
	@apksigner sign --ks /home/carlos/android-tools/debug.keystore --ks-pass pass:android \
		--ks-key-alias androiddebugkey --key-pass pass:android dist/caddin-v1.4.0.apk
	@echo "[APK] APK nativo com tela gerado e assinado: dist/caddin-v1.4.0.apk"

dist: all windows
	@rm -rf dist
	@mkdir -p dist/caddin-linux-x86_64 dist/caddin-windows-x86_64
	@cp build/caddin build/caddin_monitor .caddinrc.example README.md dist/caddin-linux-x86_64/
	@cp build-windows/caddin.exe build-windows/caddin_monitor.exe .caddinrc.example README.md dist/caddin-windows-x86_64/
	@tar -czf dist/caddin-v1.4.0-linux-x86_64.tar.gz -C dist caddin-linux-x86_64
	@cd dist && zip -r caddin-v1.4.0-windows-x86_64.zip caddin-windows-x86_64
	@echo "[DEPLOY] Pacotes gerados em dist/"

clean:
	rm -rf build build-windows build-android dist

run: all
	./build/caddin

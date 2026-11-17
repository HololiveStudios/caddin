# Compilação e Build — Caddin (v1.0.2)

O **Caddin** é construído em C11 modular sem depender de bibliotecas gráficas pesadas (como OpenGL, Mesa ou GLFW).

---

## 🐧 Compilação no Linux (Nativo X11 + XShm)

### Dependências
No Debian, Ubuntu e derivados:
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libx11-dev libxext-dev
```

### Compilar via Make
```bash
make
```

### Compilar manualmente via CMake
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

O binário executável será gerado em `build/caddin`.

---

## 🪟 Compilação Cruzada para Windows (Win32 GDI Nativo)

Você pode compilar o executável para Windows (`caddin.exe`) diretamente do Linux usando o MinGW-w64:

### Dependências
```bash
sudo apt install -y mingw-w64 cmake ninja-build
```

### Compilar via Make
```bash
make windows
```

### Compilar manualmente via CMake + Ninja
```bash
cmake -S . -B build-windows -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc"

cmake --build build-windows
```

O binário executável será gerado em `build-windows/caddin.exe`.

---

## 📦 Binários Pré-compilados
Executáveis prontos e otimizados estão disponíveis na pasta `bin/`:
- `bin/caddin` (Linux X11 64-bit)
- `bin/caddin.exe` (Windows 64-bit)

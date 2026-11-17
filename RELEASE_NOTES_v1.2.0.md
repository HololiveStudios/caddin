# 🐾 Caddin v1.2.0 — 2048 Neurônios, Persistência Atômica v2, Comportamentos Avançados e Percepção Multimodal

## O que há de novo na v1.2.0

Esta versão traz o maior salto arquitetural do Caddin:
1. **Escalação Neural:** A rede biológica ativa em tempo real foi expandida de 1024 para **2048 neurônios** (512 sensoriais + 1152 interneurônios associativos + 384 motores).
2. **Persistência Atômica v2 (`persist.c`):** Header `CADD`, versão 2, soma de verificação CRC32 (IEEE 802.3), escrita atômica via `rename()` e imunidade a corrupção testada com Fuzzing.
3. **Comportamentos Avançados (Lote P5):**
   - `behavior_hide_hunt.c`: Fuga e refúgio 80% atrás de bordas sob medo e caça lúdica do cursor.
   - `behavior_perch.c`: Pouso com gravidade e repouso na borda superior da janela ativa.
   - `behavior_startle_relief.c`: Resposta bifásica choque imediato (squish) seguido de alívio e suspiro relaxante.
4. **Percepção Avançada (Lote P6):**
   - `sense_mic.c`: Microfone em tempo real (ALSA/waveIn) monitorando RMS e palmas/estalos.
   - `sense_text.c`: Leitura contextual de palavras da janela ativa sem sobrecarga de OCR.

---

## 📦 Binários Nativos Disponíveis

- **`caddin-windows-x64.exe`** (418 KB): Executável estático de 64 bits para Windows (Win32 GDI + WinMM).
- **`caddin-v1.2.0-windows-x64.zip`** (170 KB): Arquivo compactado com o executável Windows.
- **`caddin-linux-x86_64`** (148 KB): Binário nativo ELF de 64 bits para Linux (X11 + ALSA).
- **`caddin-v1.2.0-linux-x86_64.tar.gz`** (64 KB): Tarball com o binário Linux.

---

## 🧪 Testes Unitários

7/7 suítes aprovadas com 100% de cobertura headless:
- `CoreTest` (Ponto fixo, Arena, RNG, Persistência v2 com Fuzz Test de 100 amostras)
- `SnnTest` (LIF, CSR esparso, delay axônico, R-STDP, STP)
- `BiologyTest` (Neuromodulação pentafatorial, CPG, predição temporal)
- `SensorsTest` (Mouse, scroll, janelas)
- `BodyTest` (11 timbres da voz emocional, paleta cromática)
- `MindLangTest` (Global Workspace WTA, léxico multimodal)
- `NewModulesTest` (Sysload, Heatmap, DMN, Bond, Explore, Hide/Hunt, Perch, Startle/Relief, Mic, Text)

<div align="center">

# 🐾 Caddin

### Mascote virtual autônomo para desktop, movido por uma rede neural pulsada (SNN) de 2048 neurônios

![Versão](https://img.shields.io/badge/versão-1.2.0-blue)
![Linguagem](https://img.shields.io/badge/linguagem-C-informational)
![Plataformas](https://img.shields.io/badge/plataformas-Linux%20%7C%20Windows-success)
![RAM](https://img.shields.io/badge/RAM-~12%20MB-brightgreen)
![Testes](https://img.shields.io/badge/testes-7%2F7%20aprovados-brightgreen)
![Dependências](https://img.shields.io/badge/OpenGL%2FMesa%2FLLVM-nenhum-lightgrey)

</div>

O **Caddin** é um companheiro virtual vivo que opera como uma **simulação neurobiológica em tempo real**. Ele ouve pelo microfone, lê o contexto da tela, reage ao mouse e às janelas, vocaliza emoções com síntese procedural e lembra de você entre sessões.

- 🧠 **Cérebro SNN** com 2048 neurônios biofísicos (LIF, R-STDP, 5 neuromoduladores)
- 🎙️ **Percepção multimodal:** mouse, scroll, janelas, luminância, carga de CPU, microfone e texto da tela
- 🫧 **Física softbody** elástica com renderizador 2D por CPU
- 🔊 **11 vozes emocionais** sintetizadas em tempo real, sem `malloc` no loop principal
- 💾 **Persistência atômica v2** com integridade CRC32
- 🪶 **Ultraleve:** ~12 MB de RAM, 100% nativo (X11 + ALSA / Win32 GDI + WinMM)

---

## 📑 Sumário

1. [Início rápido](#-início-rápido)
2. [Novidades da v1.2.0](#-novidades-da-v120)
3. [Controles e interações](#-controles-e-interações)
4. [Arquitetura](#-arquitetura)
5. [Cérebro SNN](#-cérebro-snn)
6. [Sistema de voz emocional](#-sistema-de-voz-emocional)
7. [Sensores e percepção](#-sensores-e-percepção)
8. [Comportamentos autônomos](#-comportamentos-autônomos)
9. [Persistência e sono](#-persistência-e-sono)
10. [Testes](#-testes)
11. [Estrutura do projeto](#-estrutura-do-projeto)
12. [Histórico de versões](#-histórico-de-versões)
13. [Roadmap](#-roadmap)
14. [Licença](#-licença)

---

## 🚀 Início rápido

### Linux (X11 + ALSA)

```bash
sudo apt install gcc make cmake libx11-dev libxext-dev libasound2-dev

git clone https://github.com/HololiveStudios/caddin.git
cd caddin
make          # compila
make run      # executa
```

### Windows (cross-compile via MinGW-w64, a partir do Linux)

```bash
sudo apt install gcc-mingw-w64-x86-64 ninja-build cmake
make windows
# Saída: build-windows/caddin.exe  e  bin/caddin.exe
```

O executável é estático e independente, sem necessidade de instalar runtime.

> Instruções detalhadas em [`BUILD.md`](BUILD.md).

---

## 🌟 Novidades da v1.2.0

### 🧠 Expansão neural para 2048 neurônios
O cérebro em tempo real passou de 1024 para **2048 neurônios** no laço de 60 FPS.

| Camada | Neurônios | Organização | Função |
|---|---|---|---|
| Sensoriais aferentes | **512** | 64 pools × 8 | Mouse, scroll, janelas, CPU, calor, luminância, microfone e texto da tela |
| Interneurônios associativos | **1152** | 32 pools × 36 | Plasticidade R-STDP, homeostase BCM, aprendizado emocional e habituação |
| Motores | **384** | 24 pools × 16 | Tônus muscular do softbody, postura e locomoção |

### 🛡️ Persistência v2 (`src/core/persist.c`)
- Cabeçalho binário estrito: magic `0x43414444` ("CADD"), versão 2, contagem de neurônios e **CRC32 (IEEE 802.3)**
- Escrita atômica: grava em `.tmp` e substitui via `rename()` / `MoveFileEx`
- Fuzz testado com 100 mutações aleatórias, com rejeição suave e sem travamentos

### 🎭 Novos comportamentos (lote P5)

| Arquivo | Comportamento |
|---|---|
| `behavior_hide_hunt.c` | Sob estresse alto, esconde 80% do corpo além da borda da janela mais próxima. Em bom humor, se aproxima do cursor e dá um bote lúdico |
| `behavior_perch.c` | Pousa no topo da janela ativa com gravidade natural e acompanha a janela se ela for arrastada |
| `behavior_startle_relief.c` | Resposta bifásica: susto (squish espasmódico), seguido de alívio com serotonina e suspiro (`VOICE_GRUNT_IDLE`) |

### 🎙️ Nova percepção (lote P6)

| Arquivo | Percepção |
|---|---|
| `sense_mic.c` | Microfone ambiente não-bloqueante (ALSA / waveIn), com RMS em tempo real para detectar palmas, gritos e voz |
| `sense_text.c` | Palavras e tópicos da janela em foco (`_NET_WM_NAME` no X11, `GetWindowText` no Windows) alimentando o `lexicon` |

---

## 🎮 Controles e interações

| Ação do usuário | Resposta do Caddin |
|---|---|
| Passar o cursor suavemente sobre ele | Atrito (rubbing), DA/5-HT ↑, `PURR_CONTENT` → `PURR_DEEP` |
| Sacudir rapidamente | Reflexo nociceptivo, `SQUEAK_STARTLED`, NE/cortisol ↑, squish |
| Sacudir continuamente | `EMO_ANNOYED` → `GROWL_ANNOYED` → `HISS_ANGRY` (escalonado) |
| Rolar a roda do mouse | Vento de rolagem, recuo do corpo, olhos fechados |
| Arrastar e arremessar | Física balística, gravidade e amortecimento elástico no solo |
| Aproximar do botão fechar | Quina de perigo: aversão ativa e esquiva |
| Trocar de janela (Alt+Tab) | Sobressalto atencional, `EMO_SURPRISED`, `SQUEAK_STARTLED` |
| Ignorá-lo por muito tempo | Saudade (bond longing): `WHINE_PROTEST` a cada ~30 s |
| Inatividade prolongada | CPG desacelera, pupilas dilatam, DMN ativa (bocejo, grooming, trilo) |
| CPU sobrecarregada | Cortisol ↑, limiar de estresse menor, agitação |

---

## 🗺️ Arquitetura

### Visão geral

```
┌─────────────────────────────────────────────────────────┐
│                  CADDIN — LOOP PRINCIPAL                 │
│    60 fps  •  dt fixo  •  zero alocação dinâmica        │
└──────────────────────────┬──────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
    SENSORES          NÚCLEO NEURAL        EXPRESSÃO
  src/sense/            src/core/           src/body/
  src/behavior/                             src/animation/
        │                  │                  │
  sense_mouse         snn.c (LIF          voice_synth.c
  sense_scroll        Q8.8 / CSR)         audio_out.c
  sense_heatmap       neuromod.c          palette.c
  sense_luma          cpg.c               softbody.c
  sense_sysload       predict.c           animation.c
  sense_windows       emotions.c
  sense_mic           workspace.c
  sense_text          lexicon.c
```

### Fluxo de consciência e vocalização

```
Estímulo (mouse, scroll, sacudida, luma, sysload, mic, texto)
        │
        ▼
  sense_* → SNN → neuromod → EmotionMachine (state_timer)
        │
        ├── EMO_AFFECTION ──► PURR_CONTENT / PURR_DEEP / TRILL
        ├── EMO_CURIOUS   ──► CHIRP_CURIOUS
        ├── EMO_SURPRISED ──► SQUEAK_STARTLED
        ├── EMO_ANNOYED   ──► GROWL_ANNOYED → HISS_ANGRY
        ├── EMO_DIZZY     ──► WHINE_PROTEST
        ├── DMN_YAWN      ──► GRUNT_IDLE
        └── Bond longing  ──► WHINE_PROTEST
        │
        ▼
  voice_synth_render() → audio_out_write() → ALSA / WinMM
```

### Módulos por diretório

#### `src/core/` — Núcleo matemático e neural

| Arquivo | Responsabilidade |
|---|---|
| `fixed.h` | Aritmética de ponto fixo Q8.8 (`fix16_t`), sem `float` no hot path |
| `snn.c/.h` | SNN LIF com CSR esparso, anel de atraso sináptico de 8 slots, R-STDP trifatorial e STP Tsodyks-Markram |
| `neuromod.c/.h` | 5 neuromoduladores (DA, NE, 5-HT, ACh, cortisol) com homeostase contínua |
| `cpg.c/.h` | CPG de respiração, batimento cardíaco e febre cognitiva |
| `predict.c/.h` | Codificação preditiva temporal e mismatch spikes |
| `persist.c/.h` | Persistência atômica v2 com CRC32 |
| `arena.c/.h` | Alocador de arena estático (zero `malloc` em runtime) |
| `rng.c/.h` | PRNG determinístico leve |

#### `src/sense/` — Sensores

| Arquivo | O que detecta |
|---|---|
| `sense_mouse` | Velocidade, aceleração, sacudida (`shake_reversals ≥ 3`), atrito e vetor relativo ao pet |
| `sense_scroll` | Vento de rolagem com decaimento exponencial |
| `sense_windows` | Topologia de janelas, galhos de pouso, quina de perigo, Alt+Tab e retângulo da janela ativa |
| `sense_heatmap` | Mapa térmico de visitas do cursor |
| `sense_luma` | Luminância média da janela ativa (XGetImage / GDI) |
| `sense_sysload` | Carga de CPU (`/proc/stat` no Linux, `GetSystemTimes` no Windows) |
| `sense_mic` | Áudio ambiente: RMS, palmas, voz |
| `sense_text` | Palavras e títulos da janela em foco |

#### `src/behavior/` — Comportamentos

| Arquivo | Comportamento |
|---|---|
| `behavior_dmn` | Default Mode Network (bocejo, grooming, micro-chirp, olhar ao redor) |
| `behavior_bond` | Vínculo de longo prazo e detecção de saudade |
| `behavior_explore` | Exploração territorial via heatmap |
| `behavior_hide_hunt` | Esconder-se sob estresse e caça lúdica do cursor |
| `behavior_perch` | Pouso sobre a janela ativa |
| `behavior_startle_relief` | Resposta bifásica susto → alívio |

#### `src/body/`, `src/animation/`, `src/mind/`, `src/lang/`

| Arquivo | Responsabilidade |
|---|---|
| `voice_synth` | 11 vozes emocionais (sine, AM, FM, vibrato, tremolo, ruído xorshift32, harmônicos, sweep, ADSR) |
| `audio_out` | Saída não-bloqueante (ALSA / WinMM), ring buffer de 8192 amostras e thread dedicada |
| `palette` | Vetor neuroquímico → aura cromática RGB e dilatação pupilar |
| `softbody` | Massa-mola: `squish`, `stretch`, `jiggle`, `purr` |
| `animation` | Interpolação de poses e sincronia com o CPG |
| `autonomy` | Locomoção guiada pelo output motor da SNN |
| `emotions` | Máquina de estados emocionais com `state_timer` |
| `workspace` | Global Workspace Theory: competição WTA, habituação e broadcast atencional |
| `lexicon` | Léxico multimodal emergente (palavra ↔ emoção), consolidação no sono e pruning |

#### `src/platform/` e `src/renderer/`

- **Linux:** X11 com XShm (memória compartilhada), cursor X11 e janelas ARGB compostas
- **Windows:** Win32 GDI com `UpdateLayeredWindow` e `BitBlt` em DC offscreen
- **Renderer:** rasterizador por software (CPU), sem OpenGL

---

## 🧠 Cérebro SNN

- **Motor:** `src/core/snn.c`, em ponto fixo Q8.8 (`fix16_t`), representação esparsa **CSR** e atraso axônico
- **Plasticidade:** R-STDP trifatorial, homeostase BCM, STP Tsodyks-Markram e habituação pré-sináptica
- **Desempenho:** menos de **3 MB de RAM** e mais de **2500 ticks/s** em um único core
- **Neuromodulação:** dopamina, noradrenalina, serotonina, acetilcolina e cortisol

---

## 🎙️ Sistema de voz emocional

Onze vozes procedurais inspiradas em vocalizações felinas e de mamíferos. Toda a síntese acontece in-place na struct `VoiceSynth`, **sem `malloc`** no loop principal.

| Tipo | Emoção | Técnica de síntese |
|---|---|---|
| `VOICE_PURR_CONTENT` | Carinho moderado | Carrier 68 Hz + AM triangular 20 Hz + harmônico de oitava + ruído 4% |
| `VOICE_PURR_DEEP` | Êxtase prolongado | 55 Hz + 3 harmônicos + AM profunda 24 Hz + vibrato de êxtase |
| `VOICE_TRILL` | Alegria / brincadeira | 3 chirps 450→780 Hz (65 ms cada) com gap de 40 ms |
| `VOICE_CHIRP_HAPPY` | Toque gostoso | 2 notas (500 / 660 Hz) com pausa entre elas |
| `VOICE_CHIRP_CURIOUS` | Curiosidade | Sweep 380→760 Hz, com vibrato nos últimos 35% |
| `VOICE_GRUNT_IDLE` | Bocejo / despertar | Descida 320→160 Hz, envelope de bocejo e ruído de rouquidão |
| `VOICE_WHINE_PROTEST` | Saudade / reclamação | Descida com vibrato crescente e tremolo pulsante |
| `VOICE_GROWL_ANNOYED` | Raiva baixa | 160 Hz + 4 harmônicos (inarmônico, textura gutural) + tremolo |
| `VOICE_HISS_ANGRY` | Raiva intensa | Ruído branco (xorshift32) com decay quadrático, sem componente tonal |
| `VOICE_SQUEAK_STARTLED` | Susto agudo | 1300→550 Hz, ataque zero e decay instantâneo |
| `VOICE_CRY_DISTRESS` | Angústia | Vibrato ±40 Hz a 7.5 Hz + tremolo + soluço a 3.5 Hz |

### Mapeamento emocional escalonado

A intensidade da voz é modulada pelo **tempo na emoção** (`state_timer`):

```
EMO_AFFECTION
  < 1s   → VOICE_CHIRP_HAPPY    (surpresa gostosa de ser tocado)
  1–4s   → VOICE_PURR_CONTENT   (carinho moderado, intensidade ∝ state_timer)
  > 4s   → VOICE_PURR_DEEP      (êxtase) + VOICE_TRILL esporádico

EMO_CURIOUS     → VOICE_CHIRP_CURIOUS
EMO_SURPRISED   → VOICE_SQUEAK_STARTLED
EMO_ANNOYED
  < 2s   → VOICE_GROWL_ANNOYED
  > 2s   → VOICE_HISS_ANGRY
EMO_DIZZY       → VOICE_WHINE_PROTEST
DMN_YAWN        → VOICE_GRUNT_IDLE
DMN_MICRO_CHIRP → VOICE_TRILL
Bond longing    → VOICE_WHINE_PROTEST
```

### API

```c
/* Trigger rico: intensity [0.0..1.0] modula volume, vibrato e profundidade */
voice_synth_trigger_ex(&voice, VOICE_PURR_DEEP, 0.50f, 0.9f);

/* Trigger simples (compatibilidade retroativa) */
voice_synth_trigger(&voice, VOICE_CHIRP_CURIOUS, 0.18f);
```

### Struct `VoiceSynth`

```c
typedef struct {
    VoiceType current_type;
    uint32_t  sample_rate, sample_index, total_samples;
    float     f0, f1, f2;          /* osciladores principal / alvo / formante */
    float     phase, mod_phase, phase2;
    float     vibrato_phase, vibrato_rate, vibrato_depth;
    float     tremolo_phase, tremolo_rate;
    uint32_t  noise_seed;           /* xorshift32 para textura orgânica */
    uint8_t   repeat_count, repeat_max;
    uint32_t  repeat_len, gap_len;  /* trilos e chirps múltiplos */
    float     intensity;            /* 0..1: modula volume e expressividade */
    float     breath_phase;
} VoiceSynth;
```

### Cooldowns por tipo

| Cooldown | Valor | Propósito |
|---|---|---|
| `VOICE_COOLDOWN_PURR` | 3.0 s | Ronron sustentado sem saturar |
| `VOICE_COOLDOWN_CHIRP` | 1.5 s | Pios de curiosidade e felicidade |
| `VOICE_COOLDOWN_ALARM` | 2.0 s | Squeak e hiss de susto/raiva |
| `VOICE_COOLDOWN_GROWL` | 2.5 s | Growl moderado |
| `VOICE_COOLDOWN_TRILL` | 4.0 s | Trilo de alegria espaçado |
| `VOICE_COOLDOWN_WHINE` | 5.0 s | Reclamação e saudade pouco frequentes |
| `VOICE_COOLDOWN_GRUNT` | 20.0 s | Bocejo raro e orgânico |

---

## 🌐 Sensores e percepção

- **Mouse:** velocidade e aceleração (px/s, px/s²), sacudida com intensidade escalonada, rubbing sobre o corpo liberando dopamina e vetor relativo ao centro do pet
- **Scroll:** vento da roda do mouse que influencia a pose (recuo, olhos fechados, desvio corporal)
- **Heatmap:** grade de visitas do cursor, usada por `behavior_explore` para buscar zonas frias
- **Luminância:** pupilas contraem com luma alta e dilatam no escuro
- **Carga do sistema:** CPU alta eleva o cortisol (estresse)
- **Janelas:** galhos de pouso, quina de perigo (botão fechar), Alt+Tab e retângulo da janela ativa
- **Microfone:** RMS ambiente para detectar palmas, gritos e voz
- **Texto da tela:** palavras e títulos da janela em foco alimentando o léxico, sem OCR

---

## 🤖 Comportamentos autônomos

- **Default Mode Network (DMN):** só ativa na inatividade. Estados: `DMN_YAWN` (stretch + grunt), `DMN_GROOMING` (jiggle), `DMN_MICRO_CHIRP` (trilo espontâneo) e `DMN_LOOK_AROUND`
- **Vínculo (bond):** acumula tempo de interação, detecta saudade (`behavior_bond_is_longing`) e reforça o apego após sacudidas
- **Exploração:** aprende posições favoritas e direciona o movimento para regiões não visitadas
- **Esconder e caçar:** foge sob estresse e caça o cursor com bom humor
- **Pouso:** usa o topo da janela ativa como galho
- **Susto → alívio:** choque imediato seguido de relaxamento com serotonina

---

## 💾 Persistência e sono

- A cada 60 s e ao fechar, pesos plásticos e estados hormonais são gravados em `caddin.brain`
- A gravação é atômica e validada por CRC32 (veja a [v1.2.0](#-novidades-da-v120))
- Durante o sono, `lexicon_consolidate_sleep` remove ruídos e consolida memórias salientes

---

## 🧪 Testes

```bash
make test
```

```text
1/7 Test #1: CoreTest .......... Passed
2/7 Test #2: SnnTest ........... Passed
3/7 Test #3: BiologyTest ....... Passed
4/7 Test #4: SensorsTest ....... Passed
5/7 Test #5: BodyTest .......... Passed
6/7 Test #6: MindLangTest ...... Passed
7/7 Test #7: NewModulesTest .... Passed

100% tests passed, 0 tests failed out of 7
```

| Teste | Cobertura |
|---|---|
| `test_core.c` | Aritmética Q8.8, arena allocator, RNG |
| `test_snn.c` | SNN LIF, CSR, R-STDP, STP Tsodyks-Markram |
| `test_biology.c` | Neuromoduladores, CPG, predição temporal |
| `test_sensors.c` | Mouse, scroll, janelas, heatmap, luma, sysload |
| `test_body.c` | 11 vozes (duração, energia, sem clipping, idle garantido) e paleta cromática |
| `test_mind_lang.c` | Global Workspace, lexicon, consolidação no sono |
| `test_new_modules.c` | DMN, bond, explore, audio_out |

---

## 📁 Estrutura do projeto

```
caddin/
├── BUILD.md                  # Build, dependências e cross-compile
├── CMakeLists.txt            # Configuração CMake
├── Makefile                  # Atalhos: make, make run, make test, make windows
├── README.md
├── RELEASE_NOTES_v1.1.6.md
├── install.sh                # Setup e instalação de pacotes
├── caddin.brain              # Estado sináptico (gerado em runtime)
├── behavior.c/.h             # Instintos base
├── conditioning.c/.h         # Condicionamento clássico e operante
├── homeostasis.c/.h          # Balanço homeostático
├── memory.c/.h               # Memória episódica e consolidação
├── network_data.h            # Topologia de rede e estruturas neurais
├── neuro.c/.h                # Módulo neuromórfico auxiliar
├── runtime_types.h           # Tipos fundamentais do runtime
├── sensors.c/.h              # Interface agregadora sensorial
├── cmake/
│   └── mingw-w64.cmake       # Toolchain para cross-compile Windows
├── docs/
│   └── PLANO_MESTRE_EVOLUCAO_CADDIN.md
├── src/
│   ├── main.c                # Loop principal (60 FPS, dt fixo, zero malloc)
│   ├── config.h              # Constantes, física, janelas e limiares
│   ├── animation/            # animation, autonomy, emotions, softbody
│   ├── behavior/             # bond, dmn, explore, hide_hunt, perch, startle_relief
│   ├── body/                 # audio_out, palette, voice_synth
│   ├── core/                 # arena, cpg, fixed, neuromod, persist, predict, rng, snn
│   ├── lang/                 # lexicon
│   ├── mind/                 # workspace
│   ├── platform/             # window.h, window_win32, window_x11, x11_cursor
│   ├── renderer/             # face, pixel_draw
│   └── sense/                # heatmap, luma, mic, mouse, scroll, sysload, text, windows
└── tests/                    # test_biology, test_body, test_core, test_mind_lang,
                              # test_new_modules, test_sensors, test_snn
```

---

## 📋 Histórico de versões

| Versão | Neurônios | Arquitetura | Destaque |
|---|---|---|---|
| **v1.2.0** | **2048** (512 + 1152 + 384) | SNN escalonada, persistência v2 CRC32, multimodal | Hide/hunt, perch, susto→alívio, microfone e leitura de texto |
| **v1.1.6** | 1024 (256 + 768) | SNN LIF com CSR escalável a 2048+ | 11 vozes emocionais, `voice_synth_trigger_ex()`, mapeamento por `state_timer` |
| **v1.1.5** | 1024 | Áudio desacoplado do motor, modulação emocional | Voz ligada às emoções, cooldowns por tipo, fix do reflexo nociceptivo |
| **v1.1.4** | 1024 | Injeção aferente de 6 sensores, DMN/Bond | ALSA/WinMM non-blocking, scroll, janela ativa, 3 comportamentos autônomos |
| **v1.1.3** | 1024 (loop) / 2048 (benchmark CSR) | Q8.8, CSR, R-STDP trifatorial, atraso sináptico | CPG marcapasso, Global Workspace, predição temporal |
| **v1.0.2** | 1024 (denso) | Matriz sináptica clássica com BCM | Release inicial: softbody e backends X11/Win32 |

---

## 🗓️ Roadmap

### ✅ Concluído até a v1.2.0

- **P1:** correção de bugs críticos (áudio mudo, centro de carinho, scroll wheel, colisão de janela)
- **P2:** bioacústica completa (11 vozes, intensidade adaptativa, cooldowns)
- **P3:** sensores e comportamentos autônomos (`sysload`, `luma`, `heatmap`, DMN, bond, explore)
- **P4:** persistência v2 com escrita atômica e CRC32
- **P5:** comportamentos avançados (hide/hunt, perch, susto→alívio)
- **P6:** percepção multimodal (microfone e leitura de texto)
- **Escalonamento neural** para 2048 neurônios

---

## 📄 Licença

Projeto acadêmico / experimental, desenvolvido por **HololiveStudios**.

<div align="center">

*Caddin v1.2.0, compilado e testado em Linux (GCC, ALSA) e Windows (MinGW-w64, WinMM)*

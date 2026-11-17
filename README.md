# 🐾 Caddin — v1.2.0

### Mascote Virtual Autônomo com SNN de 2048 Neurônios, Persistência Atômica v2, Comportamentos Avançados e Percepção Multimodal

O **Caddin** é um companheiro virtual vivo para desktop que opera como uma **simulação neurobiológica ativa** em tempo real. Com um **cérebro SNN de 2048 neurônios biofísicos ativos**, o mascote integra percepção acústica de microfone, leitura de tela contextual, física elástica softbody, árvore de comportamentos autônomos (esconder-se, caçar, pousar em janelas, ciclo susto→alívio, DMN) e **persistência atômica v2 com integridade CRC32**.

Com um **motor 2D ultraleve**, janela translúcida flutuante e **consumo de memória ultrabaixo (~12 MB de RAM total)**, roda 100% nativo em **Linux (X11 + ALSA)** e **Windows (Win32 GDI + WinMM)** sem OpenGL, Mesa, LLVM ou bibliotecas pesadas.

---

## 📑 Sumário

1. [Destaques da Versão 1.2.0](#-destaques-da-versão-120)
2. [Histórico de Versões e Contagem de Neurônios](#-histórico-de-versões-e-contagem-de-neurônios)
3. [Diagramas de Arquitetura](#-diagramas-de-arquitetura)
4. [Arquitetura Detalhada dos Módulos](#-arquitetura-detalhada-dos-módulos)
5. [Sistema de Voz Emocional (11 Tipos)](#-sistema-de-voz-emocional--11-tipos)
6. [Sensores do Desktop e Percepção Avançada](#-sensores-do-desktop-e-percepção-avançada)
7. [Comportamentos Autônomos e Límbicos](#-comportamentos-autônomos-e-límbicos)
8. [Persistência v2 com Escrita Atômica e CRC32](#-persistência-v2-com-escrita-atômica-e-crc32)
9. [Instalação e Compilação](#-instalação-e-compilação)
10. [Controles e Dinâmica Física](#-controles-e-dinâmica-física)
11. [Suíte de Testes (7 Suítes, 100% Aprovadas)](#-suíte-de-testes-7-suítes-100-aprovadas)
12. [Estrutura do Projeto Completa](#-estrutura-do-projeto-completa)

---

## 🌟 Destaques da Versão 1.2.0

### 🧠 Expansão Neural para 2048 Neurônios Ativos
O cérebro em tempo real foi escalonado de 1024 para **2048 neurônios biológicos** no laço de 60 FPS:
- **512 Neurônios Sensoriais Aferentes (64 pools × 8 neurônios):** Recebem estímulos de mouse, scroll wind, janelas, estresse de CPU, calor, luminância da tela, áudio ambiente do microfone e texto contextual de tela.
- **1152 Interneurônios Associativos (32 pools × 36 neurônios):** Duplicam a capacidade plástica com R-STDP e homeostase BCM para aprendizagem emocional e habituação de longo prazo.
- **384 Neurônios Motores (24 pools × 16 neurônios):** Maior precisão no tônus muscular do softbody, postura e locomoção autônoma.

### 🛡️ Persistência v2 com Escrita Atômica e CRC32 (`src/core/persist.c`)
- **Cabeçalho Binário Estrito:** Identificador mágico `0x43414444` ("CADD"), versão 2, contagem exata de neurônios e soma de verificação **CRC32 (IEEE 802.3)**.
- **Escrita Atômica:** Gravação preliminar em arquivo `.tmp` seguida de substituição via `rename()` / `MoveFileEx`, eliminando qualquer risco de arquivo corrompido em quedas de energia.
- **Fuzz Testing:** Testado contra 100 mutações aleatórias com garantia de rejeição suave sem nenhum travamento ou falha de segmentação.

### 🎭 Novos Comportamentos Avançados (Lote P5)
- **`behavior_hide_hunt.c` (Esconder-se e Caçar):** Sob estresse alto, o pet busca refúgio e esconde 80% do corpo além da borda da janela ativa mais próxima. Em bom humor, aproxima-se furtivamente do cursor e desfere um bote lúdico.
- **`behavior_perch.c` (Janelas como Galhos):** Pousa suavemente no topo da janela ativa em repouso com gravidade natural, acompanhando a janela se arrastada.
- **`behavior_startle_relief.c` (Resposta Bifásica Susto → Alívio):** Fase 1 (choque imediato com squish espasmódico) seguida automaticamente pela Fase 2 (alívio progressivo, injeção de serotonina e suspiro relaxante `VOICE_GRUNT_IDLE`).

### 🎙️ Percepção Avançada (Lote P6)
- **`sense_mic.c` (Microfone Ambiente):** Captura não-bloqueante (ALSA no Linux, waveIn no Windows) monitorando RMS em tempo real para detectar palmas, gritos e voz.
- **`sense_text.c` (Leitura Contextual de Tela):** Extrai palavras e tópicos da janela em foco (via `_NET_WM_NAME` no X11 e `GetWindowText` no Windows) alimentando o `lexicon` multimodal.

---

## 🌟 Destaques da Versão 1.1.6

### 🎙️ Síntese de Voz Emocional Rica (novo em v1.1.6)

11 tipos de voz procedural com técnicas de síntese biologicamente inspiradas em vocalizações felinas/mamíferas, **sem malloc** no loop principal — toda síntese é in-place na `VoiceSynth` struct:

| Tipo | Emoção | Técnica de Síntese |
|---|---|---|
| `VOICE_PURR_CONTENT` | Carinho moderado | Carrier 68 Hz + AM triangular 20 Hz + harmônico oitava + ruído 4% |
| `VOICE_PURR_DEEP` | Êxtase prolongado | 55 Hz + 3 harmônicos + AM profunda 24 Hz + vibrato de êxtase |
| `VOICE_TRILL` | Alegria / brincadeira | 3 chirps rápidos 450→780 Hz (65 ms cada) com gap de 40 ms in-place |
| `VOICE_CHIRP_HAPPY` | Toque gostoso | 2 notas (500 Hz / 660 Hz) com pausa entre elas |
| `VOICE_CHIRP_CURIOUS` | Curiosidade | Sweep 380→760 Hz; vibrato entra nos últimos 35% |
| `VOICE_GRUNT_IDLE` | Bocejo / despertar | Descida 320→160 Hz, envelope de bocejo, ruído de rouquidão |
| `VOICE_WHINE_PROTEST` | Saudade / reclamação | Descida com vibrato crescente + tremolo pulsante |
| `VOICE_GROWL_ANNOYED` | Raiva baixa | 160 Hz + 4 harmônicos (inarmônico = textura gutural) + tremolo rasgar |
| `VOICE_HISS_ANGRY` | Raiva intensa | Ruído branco puro (xorshift32) com decay quadrático — sem componente tonal |
| `VOICE_SQUEAK_STARTLED` | Susto agudo | 1300→550 Hz, ataque zero, decay instantâneo |
| `VOICE_CRY_DISTRESS` | Angústia | Vibrato intenso ±40 Hz a 7.5 Hz + tremolo + soluço a 3.5 Hz |

### 🧠 Mapeamento Emocional Escalado

A emoção determina o tipo de voz **e a intensidade é modulada pelo tempo na emoção**:

```
EMO_AFFECTION
  < 1s   → VOICE_CHIRP_HAPPY    (surpresa gostosa de ser tocado)
  1–4s   → VOICE_PURR_CONTENT   (carinho moderado, intensidade ∝ state_timer)
  > 4s   → VOICE_PURR_DEEP      (êxtase) + VOICE_TRILL esporádico

EMO_CURIOUS   → VOICE_CHIRP_CURIOUS  (pio interrogativo ascendente)
EMO_SURPRISED → VOICE_SQUEAK_STARTLED (susto agudo instantâneo)
EMO_ANNOYED
  < 2s   → VOICE_GROWL_ANNOYED (raiva crescente)
  > 2s   → VOICE_HISS_ANGRY    (raiva intensa)
EMO_DIZZY     → VOICE_WHINE_PROTEST  (reclamação suave de tontura)
DMN_YAWN      → VOICE_GRUNT_IDLE     (bocejo orgânico e sonolento)
DMN_MICRO_CHIRP → VOICE_TRILL        (vivacidade autônoma)
Bond longing  → VOICE_WHINE_PROTEST  (saudade de quem está sendo ignorado)
```

### 🔊 API de Voz com Intensidade

```c
/* Trigger rico — intensity [0.0..1.0] modula volume, vibrato e profundidade */
voice_synth_trigger_ex(&voice, VOICE_PURR_DEEP, 0.50f, 0.9f);

/* Trigger simples (compatibilidade retroativa) */
voice_synth_trigger(&voice, VOICE_CHIRP_CURIOUS, 0.18f);
```

---

## 📋 Histórico de Versões e Contagem de Neurônios

| Versão | Tag | Neurônios Ativos | Topologia / Arquitetura Neural | Destaque Principal |
|---|---|---|---|---|
| **v1.2.0** | `v1.2.0` | **2048** (512 sensoriais + 1152 inter + 384 motores) | SNN Escalonada + Persistência v2 CRC32 + Multimodal | Expansão para 2048 neurônios, persistência atômica v2, comportamentos hide/hunt/perch/alívio, microfone e leitura de texto |
| **v1.1.6** | `v1.1.6` | **1024** (256 sensoriais + 768 inter/motores) | SNN Biofísica LIF + CSR esparso escalável a **2048+** neurônios | 11 tipos de voz emocional procedural, `voice_synth_trigger_ex()`, mapeamento escalado por `state_timer` |
| **v1.1.5** | `v1.1.5` | **1024** (256 sensoriais + 768 inter/motores) | Desacoplamento motor de áudio + modulação emocional | Vocalização vinculada estritamente a emoções (fim do grito), cooldowns por tipo, fix reflexo nociceptivo |
| **v1.1.4** | `v1.1.4` | **1024** (256 sensoriais + 768 inter/motores) | Injeção aferente de 6 sensores periféricos + DMN/Bond | ALSA/WinMM non-blocking, scroll wheel, janela ativa, 3 novos sensores, 3 comportamentos autônomos |
| **v1.1.3** | `v1.1.3` | **1024** (loop) / **2048** (benchmark CSR) | Ponto fixo Q8.8, CSR, R-STDP trifatorial, atraso sináptico | Integração completa no loop principal, CPG marcapasso, Global Workspace e predição temporal |
| **v1.0.2** | — | **1024** neurônios densos | Matriz sináptica clássica com plasticidade BCM | Release inicial — SNN 1024, Softbody elástico, backends X11/Win32 |

### 🧠 Detalhamento da Topologia Neural Atual (v1.2.0)

- **Loop Principal em Tempo Real (`PESTINHA_NEURONS = 2048`):**
  - **512 Neurônios Sensoriais Aferentes (64 pools × 8 neurônios):** Mapeiam cinemática do mouse, atrito de carinho, vento de scroll, calor de exploração, luminância da tela (luma), estresse de CPU (sysload), geometria de janelas, áudio do microfone e palavras de tela.
  - **1152 Interneurônios Associativos (32 pools × 36 neurônios):** Duplicam a densidade de conexões recorrentes com homeostase BCM, habituação pré-sináptica e decaimento dinâmico.
  - **384 Neurônios Motores (24 pools × 16 neurônios):** Aumentam a granularidade do tônus muscular (squish, stretch, jiggle, purr) e locomoção intencional.
- **Motor SNN de Próxima Geração CSR (`src/core/snn.c`):**
  - Implementado em aritmética de ponto fixo Q8.8 (`fix16_t`) com representação esparsa CSR (Compressed Sparse Row) e atraso axônico.
  - Consome menos de **3 MB de RAM** e executa a mais de **2500 ticks/s** em apenas 1 core de CPU.

---

## 🗺️ Diagramas de Arquitetura

### Diagrama de Blocos Modular

```
┌─────────────────────────────────────────────────────────┐
│                    CADDIN — LOOP PRINCIPAL               │
│  60 fps  •  dt fixo  •  zero alocação dinâmica no loop  │
└──────────────────────────┬──────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   SENSORES          NÚCLEO NEURAL        EXPRESSÃO
   src/sense/         src/core/            src/body/
   src/behavior/                           src/animation/
        │                  │                  │
   ─────────         ──────────         ─────────────
   sense_mouse       snn.c (SNN         voice_synth.c
   sense_scroll      LIF Q8.8/CSR)      (11 vozes
   sense_heatmap     neuromod.c         emocionais)
   sense_luma        (5 neuromod.)
   sense_sysload     cpg.c (CPG)        palette.c
   sense_windows     predict.c          audio_out.c
                     (pred. temporal)   softbody.c
        │                  │            animation.c
        │            ──────────              │
        │            emotions.c         ─────────────
        │            workspace.c        renderer/
        │            lexicon.c          platform/
        └──────────────────┴──────────────────┘
```

### Fluxo de Consciência e Vocalização

```
Estímulo físico (mouse, scroll, sacudida, luma, sysload)
        │
        ▼
  sense_* → SNN → neuromod → EmotionMachine (state_timer)
        │
        ├── em.current == EMO_AFFECTION ──► PURR_CONTENT / PURR_DEEP / TRILL
        ├── em.current == EMO_CURIOUS   ──► CHIRP_CURIOUS
        ├── em.current == EMO_SURPRISED ──► SQUEAK_STARTLED
        ├── em.current == EMO_ANNOYED   ──► GROWL_ANNOYED → HISS_ANGRY
        ├── em.current == EMO_DIZZY     ──► WHINE_PROTEST
        ├── DMN_YAWN                    ──► GRUNT_IDLE
        └── Bond longing                ──► WHINE_PROTEST
        │
        ▼
  voice_synth_render() → audio_out_write() → ALSA / WinMM
```

---

## 🔬 Arquitetura Detalhada dos Módulos

### 1. Núcleo Matemático e SNN (`src/core/`)

| Arquivo | Responsabilidade |
|---|---|
| `fixed.h` | Aritmética ponto fixo Q8.8 (`fix16_t`) — sem `float` no hot path |
| `snn.c/.h` | SNN LIF com CSR esparso, anel de atraso sináptico 8 slots, R-STDP trifatorial, STP Tsodyks-Markram |
| `neuromod.c/.h` | 5 neuromoduladores: DA, NE, 5-HT, ACh, Cortisol — homeostase contínua |
| `cpg.c/.h` | Oscilador CPG de respiração + batimento cardíaco + febre cognitiva |
| `predict.c/.h` | Codificação preditiva temporal + mismatch spikes |
| `arena.c/.h` | Alocador de arena estático (zero `malloc` em runtime) |
| `rng.c/.h` | PRNG determinístico leve |

### 2. Sensores Físicos do Desktop (`src/sense/`)

| Arquivo | O que detecta |
|---|---|
| `sense_mouse.c/.h` | Velocidade, aceleração, sacudida (`shake_reversals ≥ 3`), atrito/rubbing, vetor de posição relativa ao pet |
| `sense_scroll.c/.h` | Vento de rolagem gerado pela roda do mouse com decaimento exponencial |
| `sense_windows.c/.h` | Topologia de janelas, galhos de pouso (perching), quina de perigo (botão fechar), troca de foco (Alt+Tab), retângulo da janela ativa |
| `sense_heatmap.c/.h` | Mapa térmico de frequência de visitas do cursor no desktop |
| `sense_luma.c/.h` | Luminância média do conteúdo da janela ativa (via XGetImage / GDI) |
| `sense_sysload.c/.h` | Carga de CPU do sistema operacional (Linux: `/proc/stat`, Windows: `GetSystemTimes`) |

### 3. Comportamentos Autônomos (`src/behavior/`)

| Arquivo | Comportamento |
|---|---|
| `behavior_dmn.c/.h` | Default Mode Network: bocejo (`DMN_YAWN`), grooming, micro-chirp (`DMN_MICRO_CHIRP`), olhar ao redor (`DMN_LOOK_AROUND`) — só na inatividade |
| `behavior_bond.c/.h` | Vínculo de longo prazo: lembra tempo de interação, detecta saudade (`behavior_bond_is_longing`), responde a sacudidas com reforço de apego |
| `behavior_explore.c/.h` | Exploração territorial: aprende posições favoritas via heatmap, dirige movimento autônomo para regiões não visitadas |

### 4. Expressão Corporal e Voz Procedural (`src/body/`)

| Arquivo | Responsabilidade |
|---|---|
| `voice_synth.c/.h` | **11 tipos de voz emocional** (Q8.8 → float; síntese: sine, AM, FM, vibrato, tremolo, ruído xorshift32, harmônicos, sweep, envelope ADSR) |
| `audio_out.c/.h` | Saída de áudio non-blocking: ALSA (Linux) + WinMM (Windows), ring buffer 8192 amostras, thread dedicada |
| `palette.c/.h` | Mapeamento do vetor neuroquímico (5 neuromod.) em aura cromática RGB + dilatação pupilar |

#### Struct `VoiceSynth` — estado completo sem alocação

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
    uint32_t  repeat_len, gap_len;  /* para trilos e chirps múltiplos */
    float     intensity;            /* 0..1 — modula volume e expressividade */
    float     breath_phase;
} VoiceSynth;
```

### 5. Física Softbody e Renderizador CPU (`src/animation/`, `src/renderer/`)

| Arquivo | Responsabilidade |
|---|---|
| `softbody.c/.h` | Física massa-mola procedural: `softbody_squish`, `softbody_stretch`, `softbody_jiggle`, `softbody_purr` |
| `animation.c/.h` | Interpolação de poses, sincronia com CPG (expansão torácica visível) |
| `autonomy.c/.h` | Movimentação autônoma guiada pelo output motor da SNN |
| `emotions.c/.h` | Máquina de estados emocionais com `state_timer` para escalada comportamental |

### 6. Espaço de Trabalho Global e Linguagem (`src/mind/`, `src/lang/`)

| Arquivo | Responsabilidade |
|---|---|
| `workspace.c/.h` | Global Workspace Theory: competição WTA entre estímulos, habituação, broadcast atencional |
| `lexicon.c/.h` | Léxico multimodal emergente: associação palavra↔emoção, consolidação durante o sono, pruning de ruídos |

### 7. Camadas de Plataforma (`src/platform/`)

Backends nativos sem dependências de terceiros:
- **Linux:** X11 com XShm (memória compartilhada), cursor X11, janelas ARGB compostas
- **Windows:** Win32 GDI com `UpdateLayeredWindow`, `BitBlt` em DC offscreen

---

## 🎙️ Sistema de Voz Emocional — v1.1.6

### Técnicas de Síntese por Tipo

#### Ronrons (`PURR_CONTENT` / `PURR_DEEP`)
Inspirados na fisiologia do ronronar felino (25–150 Hz, AM 15–30 Hz):
```
Carrier sine (68 Hz / 55 Hz)
  + AM triangular (20 Hz / 24 Hz)   ← ritmo inhale/exhale
  + Harmônico oitava (20–30%)
  + Ruído branco xorshift (4–5%)    ← textura orgânica
  + Vibrato ±3 Hz (apenas DEEP)     ← ondulação de êxtase
```

#### Trilos e Pios Felizes (`TRILL` / `CHIRP_HAPPY`)
Gestão in-place de repetições via `repeat_len` / `gap_len`:
```
TRILL:       [chirp 450→780 Hz / 65 ms] × 3  com gap 40 ms
CHIRP_HAPPY: [nota 500 Hz / 70 ms] + [gap 25 ms] + [nota 660 Hz / 70 ms]
```

#### Growl e Hiss (`GROWL_ANNOYED` / `HISS_ANGRY`)
```
GROWL: f0 (160–220 Hz) + h2 (50%) + h3 (30%) + h4×4.5 inarmônico (15%)
       + tremolo 11 Hz (rasgar de garganta) + ruído 10%
HISS:  noise_next(xorshift32) com decay quadrático — puramente fricativo
```

### Cooldowns por Tipo de Voz

| Cooldown | Valor | Propósito |
|---|---|---|
| `VOICE_COOLDOWN_PURR` | 3.0 s | Ronron sustentado, não satura |
| `VOICE_COOLDOWN_CHIRP` | 1.5 s | Pios de curiosidade/felicidade |
| `VOICE_COOLDOWN_ALARM` | 2.0 s | Squeak/hiss de susto/raiva |
| `VOICE_COOLDOWN_TRILL` | 4.0 s | Trilo de alegria espaçado |
| `VOICE_COOLDOWN_WHINE` | 5.0 s | Reclamação/saudade pouco frequente |
| `VOICE_COOLDOWN_GROWL` | 2.5 s | Growl de raiva moderado |
| `VOICE_COOLDOWN_GRUNT` | 20.0 s | Bocejo: raro e orgânico |

---

## 🌐 Sensores do Desktop

### Mouse (`sense_mouse.c`)
- Velocidade e aceleração em px/s e px/s²
- Sacudida (`shake_reversals ≥ 3` inversões de direção) com intensidade escalonada
- Rubbing/atrito: cursor lento sobre o corpo do pet → liberação de dopamina
- Vetor relativo ao centro do pet para física direcional

### Scroll Wind (`sense_scroll.c`)
- Vento gerado pela roda do mouse com decaimento exponencial
- Influencia a pose do pet (recuar, fechar olhos, desvio corporal)

### Mapa Térmico (`sense_heatmap.c`)
- Grade de frequência de visitas do cursor no desktop
- `behavior_explore` usa para direcionar exploração para zonas frias

### Luminância (`sense_luma.c`)
- Captura a luminância média da janela ativa (XGetImage / GDI BitBlt)
- Pupilas contraem com luma alta, dilatam no escuro

### Carga do Sistema (`sense_sysload.c`)
- CPU load via `/proc/stat` (Linux) ou `GetSystemTimes` (Windows)
- Sistema sobrecarregado → neuromodulação de estresse (cortisol ↑)

### Janelas do Desktop (`sense_windows.c`)
- Galhos de pouso: bordas superiores de janelas como superfície de descanso
- Quina de perigo: botão fechar (canto superior direito) gera aversão ativa
- Troca de foco (Alt+Tab): sobressalto atencional
- Retângulo da janela ativa para colisão e navegação

---

## 🤖 Comportamentos Autônomos

### Default Mode Network (`behavior_dmn.c`)
Ativa apenas durante inatividade do usuário. Estados:
- **`DMN_YAWN`** — bocejo com `softbody_stretch` + `VOICE_GRUNT_IDLE`
- **`DMN_GROOMING`** — auto-limpeza com `softbody_jiggle`
- **`DMN_MICRO_CHIRP`** — vocalização espontânea `VOICE_TRILL`
- **`DMN_LOOK_AROUND`** — reorientação do olhar

### Vínculo de Longo Prazo (`behavior_bond.c`)
- Rastreia tempo acumulado de interação por sessão
- Detecta saudade (`behavior_bond_is_longing`) → `VOICE_WHINE_PROTEST` a cada ~30s
- Sacudidas reforçam o apego (resposta de protesto → retorno de atenção)

### Exploração Territorial (`behavior_explore.c`)
- Aprende posições favoritas via `sense_heatmap`
- Dirige movimento autônomo para regiões não visitadas do desktop

---

## 🛠️ Instalação e Compilação

### Dependências

**Linux:**
```bash
sudo apt install gcc make cmake libx11-dev libxext-dev libasound2-dev
```

**Windows (compilação cruzada a partir do Linux):**
```bash
sudo apt install gcc-mingw-w64-x86-64 ninja-build cmake
```

---

### Linux (Nativo X11 + ALSA)

```bash
git clone https://github.com/HololiveStudios/caddin.git
cd caddin
make          # compila
make run      # executa
./build/caddin
```

---

### Windows (Cross-compile via MinGW-w64)

```bash
make windows
# Binário: build-windows/caddin.exe  e  bin/caddin.exe
```

Executável estático independente — sem instalação de runtime.

---

### Suíte de Testes (`make test`)

```bash
make test
```

```text
Test project /home/.../caddin/build
    Start 1: CoreTest
1/7 Test #1: CoreTest .........................   Passed    0.01 sec
    Start 2: SnnTest
2/7 Test #2: SnnTest ..........................   Passed    0.33 sec
    Start 3: BiologyTest
3/7 Test #3: BiologyTest ......................   Passed    0.00 sec
    Start 4: SensorsTest
4/7 Test #4: SensorsTest ......................   Passed    0.00 sec
    Start 5: BodyTest
5/7 Test #5: BodyTest .........................   Passed    0.01 sec
    Start 6: MindLangTest
6/7 Test #6: MindLangTest .....................   Passed    0.01 sec
    Start 7: NewModulesTest
7/7 Test #7: NewModulesTest ...................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 7
```

---

## 🎮 Controles e Dinâmica Física

| Ação do Usuário | Resposta Biológica |
|---|---|
| **Passar o cursor suavemente sobre o pet** | Atrito (rubbing), DA/5-HT ↑, `VOICE_PURR_CONTENT` → `VOICE_PURR_DEEP` |
| **Sacudir rapidamente** | Arco reflexo nociceptivo, `VOICE_SQUEAK_STARTLED`, NE/Cortisol ↑, squish |
| **Sacudir continuamente** | `EMO_ANNOYED` → `VOICE_GROWL_ANNOYED` → `VOICE_HISS_ANGRY` escalonado |
| **Rolar a roda do mouse** | Vento de rolagem, recuo do corpo, olhos fechados |
| **Arrastar e arremessar** | Física balística inercial, gravidade, amortecimento elástico no solo |
| **Aproximar do botão fechar** | Sensor de quina perigosa, aversão ativa e esquiva |
| **Trocar de janela (Alt+Tab)** | Sobressalto atencional, `EMO_SURPRISED`, `VOICE_SQUEAK_STARTLED` |
| **Ignorar o pet por muito tempo** | Bond longing → `VOICE_WHINE_PROTEST` a cada ~30s |
| **Inatividade prolongada** | CPG desacelera, pupilas dilatam, DMN ativa (bocejo, grooming, trilo) |
| **CPU do sistema sobrecarregada** | Cortisol ↑, limiar de estresse mais baixo, agitação |

---

## 🧪 Suíte de Testes (7 testes, 100% aprovados)

| Teste | Cobertura |
|---|---|
| `test_core.c` | Aritmética Q8.8, arena allocator, RNG |
| `test_snn.c` | SNN LIF, CSR, R-STDP, STP Tsodyks-Markram |
| `test_biology.c` | Neuromoduladores, CPG, predição temporal |
| `test_sensors.c` | sense_mouse, sense_scroll, sense_windows, sense_heatmap, sense_luma, sense_sysload |
| `test_body.c` | **11 tipos de voz** (duração, energia, sem clipping, idle garantido), paleta cromática |
| `test_mind_lang.c` | Global Workspace, lexicon, consolidação do sono |
| `test_new_modules.c` | behavior_dmn, behavior_bond, behavior_explore, audio_out |

---

## 💾 Persistência Sináptica e Sono

O Caddin preserva aprendizado entre sessões:
- A cada 60 s ou ao fechar, pesos plásticos e estados hormonais são gravados em `caddin.brain`
- Durante o sono: `lexicon_consolidate_sleep` remove ruídos e consolida memórias salientes
- Escrita segura para prevenir corrupção de estado

---

## 📁 Estrutura do Projeto

```
caddin/
├── BUILD.md                       # Instruções detalhadas de build, dependências e compilação cruzada
├── CMakeLists.txt                 # Script mestre de configuração e compilação CMake
├── Makefile                       # Atalhos de automação (make, make run, make test, make windows)
├── README.md                      # Documentação técnica completa e guia do ecossistema
├── RELEASE_NOTES_v1.1.6.md        # Notas de lançamento detalhadas da versão v1.1.6
├── behavior.c / .h                # Camada comportamental / instintos base
├── conditioning.c / .h            # Condicionamento clássico e operante
├── homeostasis.c / .h             # Balanço homeostático e dinâmica biológica complementar
├── memory.c / .h                  # Subsistema de memória episódica e consolidação
├── network_data.h                 # Definições de topologia de rede e estruturas neurais
├── neuro.c / .h                   # Módulo neuromórfico auxiliar
├── runtime_types.h                # Tipos de dados fundamentais do runtime
├── sensors.c / .h                 # Interface agregadora sensorial
├── install.sh                     # Script shell de setup e instalação de pacotes
├── caddin.brain                   # Estado sináptico persistido (gerado em runtime)
├── cmake/
│   └── mingw-w64.cmake            # Arquivo de toolchain para cross-compilação Windows no Linux
├── docs/
│   └── PLANO_MESTRE_EVOLUCAO_CADDIN.md # Documento de engenharia e plano de expansão
├── src/
│   ├── main.c                     # Loop principal em tempo real (60 FPS, dt fixo, zero malloc)
│   ├── config.h                   # Constantes de configuração, física, janelas e limiares
│   │
│   ├── animation/                 # Física elástica, animação e dinâmica afetiva
│   │   ├── animation.c / .h       # Interpolação de poses e sincronia respiratória com CPG
│   │   ├── autonomy.c / .h        # Motor de locomoção e perambulação guiado pela SNN
│   │   ├── emotions.c / .h        # Máquina de estados emocionais e temporizadores (state_timer)
│   │   └── softbody.c / .h        # Física massa-mola procedural (squish, stretch, jiggle, purr)
│   │
│   ├── behavior/                  # Comportamentos complexos autônomos
│   │   ├── behavior_bond.c / .h   # Vínculo afetivo de longo prazo, apego e detecção de saudade
│   │   ├── behavior_dmn.c / .h    # Default Mode Network (bocejos, auto-limpeza, trilos e devaneios)
│   │   ├── behavior_explore.c / .h# Exploração territorial baseada em novidade e mapa térmico
│   │   ├── behavior_hide_hunt.c/.h# Se esconder atrás de bordas sob medo e caça furtiva de cursor
│   │   ├── behavior_perch.c / .h  # Pouso e descanso sobre o topo da janela ativa
│   │   └── behavior_startle_relief.c/.h # Resposta bifásica choque imediato -> alívio com serotonina
│   │
│   ├── body/                      # Expressão física, bioacústica e áudio
│   │   ├── audio_out.c / .h       # Saída de áudio assíncrona multithread (ALSA no Linux / WinMM no Windows)
│   │   ├── palette.c / .h         # Mapeamento do vetor neuroquímico em aura cromática e pupilas
│   │   └── voice_synth.c / .h     # Síntese procedural PCM/FM rica (11 tipos de vocalizações emocionais)
│   │
│   ├── core/                      # Núcleo neurobiológico e computacional de baixa latência
│   │   ├── arena.c / .h           # Alocador de memória Arena estático (zero fragmentação)
│   │   ├── cpg.c / .h             # Central Pattern Generator (respiração biológica e febre cognitiva)
│   │   ├── fixed.h                # Biblioteca matemática em ponto fixo Q8.8 (fix16_t)
│   │   ├── neuromod.c / .h        # Dinâmica de 5 neuromoduladores (Dopamina, Noradrenalina, Serotonina, Acetilcolina, Cortisol)
│   │   ├── persist.c / .h         # Persistência atômica v2 com integridade CRC32 e proteção contra corrupção
│   │   ├── predict.c / .h         # Codificação preditiva temporal e disparos de surpresa (mismatch spikes)
│   │   ├── rng.c / .h             # Gerador pseudo-aleatório ultra-rápido determinístico
│   │   └── snn.c / .h             # Rede Neural Pulsada (SNN) LIF com formato esparso CSR e plasticidade R-STDP/STP
│   │
│   ├── lang/                      # Aquisição e consolidação de linguagem emergente
│   │   └── lexicon.c / .h         # Léxico multimodal associativo com compressão e pruning no sono
│   │
│   ├── mind/                      # Consciência funcional e atenção
│   │   └── workspace.c / .h       # Espaço de Trabalho Global (Global Workspace Theory, competição WTA)
│   │
│   ├── platform/                  # Camadas de abstração de janela e subsistemas de SO
│   │   ├── window.h               # Interface agnóstica de controle de janela transparente
│   │   ├── window_win32.c         # Implementação Win32 GDI com UpdateLayeredWindow
│   │   ├── window_x11.c           # Implementação Linux X11 com XShm e composição ARGB
│   │   └── x11_cursor.c / .h      # Interrogação de ponteiro e janelas ativas no servidor X11
│   │
│   ├── renderer/                  # Rasterizador por software em CPU (sem dependência de OpenGL)
│   │   ├── face.c / .h            # Desenho vetorial de feições (olhos, pupila, boca, sobrancelhas)
│   │   └── pixel_draw.c / .h      # Primitivas gráficas antialiasing e preenchimento de polígonos
│   │
│   └── sense/                     # Percepção física e sensorial integrada ao SO
│       ├── sense_heatmap.c / .h   # Grade de calor espacial da densidade de uso do cursor
│       ├── sense_luma.c / .h      # Captura de luminância da tela sob o mascote
│       ├── sense_mic.c / .h       # Captura de microfone ambiente (RMS, palmas, voz)
│       ├── sense_mouse.c / .h     # Vetores de movimento, velocidade, atrito de carinho e sacudidas
│       ├── sense_scroll.c / .h    # Vento aerodinâmico gerado pelo scroll do mouse
│       ├── sense_sysload.c / .h   # Monitoramento em tempo real de carga de CPU do SO
│       ├── sense_text.c / .h      # Leitura contextual de palavras e títulos da janela ativa
│       └── sense_windows.c / .h   # Geometria de janelas (galhos de pouso, quina de risco, Alt+Tab)
│
└── tests/                         # Suíte completa de testes unitários automatizados (Headless)
    ├── test_biology.c             # Verificação de CPG, neuromoduladores e predição temporal
    ├── test_body.c                # Testes dos 11 timbres de voz procedural e paleta
    ├── test_core.c                # Testes de integridade da aritmética Q8.8, arena e RNG
    ├── test_mind_lang.c           # Verificação do Global Workspace e consolidação léxica
    ├── test_new_modules.c         # Verificação de DMN, vínculo afetivo, exploração e áudio assíncrono
    ├── test_sensors.c             # Testes de todos os sensores de desktop (mouse, janelas, scroll, luma, carga)
    └── test_snn.c                 # Validação da dinâmica SNN LIF, conectividade CSR e R-STDP
```

---

## 📋 Roadmap e Status das Prioridades

### ✅ Concluído até v1.2.0
- **🟢 P1 — Correção de Bugs Críticos:** Áudio mudo no pipeline, centro de carinho do mouse, captura de scroll wheel e colisão de janela ativa resolvidos.
- **🟢 P2 — Bioacústica e Voz Procedural Completa:** Sintetizador rico com 11 tipos emocionais, intensidade adaptativa e cooldowns estritos guiados por estados emocionais.
- **🟢 P3 — Sensores e Comportamentos Autônomos:** Integração de `sysload`, `luma`, `heatmap`, `behavior_dmn` (Default Mode Network), `behavior_bond` (vínculo afetivo) e `behavior_explore`.
- **🟢 P4 — Persistência v2 com Escrita Atômica e CRC32 (`src/core/persist.c`):**
  - Header binário com magic bytes `0x43414444` ("CADD"), versão 2 e soma de verificação CRC32 (IEEE 802.3).
  - Escrita atômica via arquivo temporário + `rename()` / `MoveFileEx`.
  - Fuzz test com 100 amostras mutantes garantindo imunidade total contra corrupção ou falhas.
- **🟢 P5 — Comportamentos Avançados:**
  - `behavior_hide_hunt.c`: Fuga e refúgio 80% atrás de bordas sob estresse alto e caça lúdica do cursor em bom humor.
  - `behavior_perch.c`: Pouso com gravidade e repouso sobre a borda superior da janela ativa.
  - `behavior_startle_relief.c`: Resposta bifásica choque imediato (squish) seguido de alívio progressivo com serotonina e suspiro.
- **🟢 P6 — Percepção Avançada Multimodal:**
  - `sense_mic.c`: Captura não-bloqueante de microfone (ALSA / waveIn) com cálculo RMS de som ambiente e detecção de palmas.
  - `sense_text.c`: Leitura contextual de palavras e títulos de janelas ativas alimentando o léxico multimodal sem sobrecarga de OCR.
- **🟢 Escalonamento Neural:** Expansão da rede biológica ativa para **2048 neurônios** (512 sensoriais + 1152 inter + 384 motores).

---

## 📄 Licença

Projeto acadêmico / experimental — HololiveStudios.

---

*Caddin v1.1.6 — compilado e testado em Linux (GCC, ALSA) e Windows (MinGW-w64, WinMM)*

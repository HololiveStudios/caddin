# 01 — Plano de Implementação Principal (Pet Digital em C)

Cobre as **30 ideias** do documento original, organizadas em fases e **lotes**. Cada lote lista os arquivos que toca; lotes que mexem nos mesmos arquivos são feitos **juntos**. Lotes de uma mesma fase marcados como "paralelo" não dependem entre si e podem ser feitos ao mesmo tempo (por pessoas diferentes ou na mesma sessão).

Orçamento e restrições: veja `00-PROMPT-MESTRE-E-PESQUISA.md` (seção 2).

---

## 1. Estrutura de pastas

```
pet/
├─ Makefile / CMakeLists.txt
├─ third_party/            # miniaudio.h, pffft.[ch], stb_*.h (vendorizados)
├─ include/pet/            # headers públicos
├─ src/
│  ├─ core/                # SEM includes do SO
│  │  ├─ arena.c/.h        # alocador de arenas
│  │  ├─ rng.c/.h          # xoshiro128** determinístico
│  │  ├─ fixed.h           # ponto fixo, saturação, LUTs (exp, sin)
│  │  ├─ snn.c/.h          # neurônios LIF, CSR, fila de eventos
│  │  ├─ plasticity.c/.h   # STDP, traços de elegibilidade, STP, scaling
│  │  ├─ neuromod.c/.h     # DA, NE, 5-HT, ACh, cortisol, ocitocina
│  │  ├─ cpg.c/.h          # respiração, batimento
│  │  ├─ homeostasis.c/.h  # febre cognitiva, energia, sono
│  │  ├─ brain.c/.h        # montagem das regiões e do pipeline por tick
│  │  ├─ persist.c/.h      # salvar/carregar cérebro, genoma
│  │  └─ log.c/.h
│  ├─ sense/               # sensores (traduzem para correntes de entrada)
│  │  ├─ sense_mouse.c, sense_caret.c, sense_scroll.c, sense_windows.c,
│  │  ├─ sense_luma.c, sense_sysload.c, sense_sysaudio.c, sense_heatmap.c
│  ├─ body/                # corpo e expressão
│  │  ├─ body.c            # física elástica, squash/stretch, pupilas
│  │  ├─ render.c          # rasterizador de software
│  │  ├─ palette.c         # aura cromática
│  │  └─ voice.c           # FM/PCM procedural
│  ├─ behavior/            # DMN, caça, esconder, território, apego, etc.
│  ├─ pal/                 # Platform Abstraction Layer
│  │  ├─ pal.h
│  │  ├─ pal_win32.c, pal_x11.c, pal_macos.m, pal_headless.c
│  └─ main.c
├─ tests/                  # unitários, determinismo, fuzz do persist
├─ bench/                  # microbenchmarks e relatório de RAM/CPU
└─ docs/DECISOES.md
```

---

## 2. Design do núcleo (decisões técnicas)

### 2.1 Neurônio (LIF em ponto fixo, SoA)

```c
// include/pet/snn.h (esboço)
typedef struct {
    int16_t  *v;        // potencial de membrana (Q8.8 ou mV*16)
    int16_t  *thr;      // limiar adaptativo (plasticidade intrínseca)
    uint8_t  *refrac;   // contador refratário em ticks
    int16_t  *i_in;     // corrente acumulada neste tick
    uint8_t  *region;   // id da região
    uint32_t  n;
    int16_t   leak_shift; // decaimento: v -= v >> leak_shift
} Neurons;

typedef struct {        // sinapses em CSR (por neurônio pré)
    uint32_t *row_ptr;  // n_pre + 1
    uint32_t *post;     // índice do pós
    int8_t   *w;        // peso (ou int16 para vias plásticas)
    uint8_t  *delay;    // atraso em ticks (0..7)
    int8_t   *elig;     // traço de elegibilidade (3 fatores)
    uint8_t  *stp_u, *stp_x; // facilitação/depressão (fadiga sináptica)
    uint32_t  n_syn;
} Synapses;
```

Passo por tick (1–2 ms):
1. **Decair** potenciais (`v -= v >> leak_shift`) apenas dos neurônios "ativos" (lista esparsa).
2. **Integrar** correntes de entrada (sensores + sinapses com atraso).
3. **Disparar** quem passou do limiar → coloca na **fila de spikes** e zera `v`, ativa refratário.
4. **Propagar** spikes via CSR (somar `w * stp_u*stp_x` no `i_in` do pós, na janela de atraso).
5. **Plasticidade**: atualizar traços pré/pós; ajustar `elig`; aplicar `Δw = elig × (dopamina − baseline)` apenas em lote a cada N ticks.
6. **Neuromoduladores**: decair, multiplicar ganho/limiar global.

### 2.2 Escala-alvo

| Região | Neurônios | Observação |
|---|---|---|
| Sensorial (mouse, scroll, luma, carga, áudio SO, caret, heatmap) | 600–1500 | População de codificação (taxa + direção) |
| Interneurônios / associativa | 1000–3000 | E/I balanceado (80/20) |
| Motora | 300–800 | Alimenta o corpo |
| Límbica/afetiva (medo, curiosidade, apego, ressentimento) | 200–600 | Pools nomeados |
| Tronco (CPG, sono) | 16–64 | Oscilatório |
| DMN | 100–300 | Ativo quando ocioso |
| **Total** | **~2.5k–6k** | ~100k–400k sinapses ⇒ **< 3 MB** |

### 2.3 Tempo e determinismo

- `tick_ms = 2` em vigília ativa; `20–50 ms` em sono/repouso (o código de decaimento usa `dt` em múltiplos de tick para manter a dinâmica correta).
- Entradas do SO entram por **fila SPSC sem lock** (`pal` → `core`) com *timestamps*.
- Modo *replay*: gravar o fluxo de eventos (mouse/janelas) em arquivo e reexecutar offline (testes e depuração).

---

## 3. Fases e lotes

Legenda: **[P]** = pode rodar em paralelo com outros [P] da mesma fase.

### FASE 0 — Fundamentos (1 lote, sequencial)

**Lote 0.1 — Esqueleto**
- Arquivos: `Makefile`, `src/core/arena.*`, `rng.*`, `fixed.h`, `log.*`, `tests/`, `bench/`, `docs/DECISOES.md`.
- Entregas: arena com `arena_alloc/reset`, RNG xoshiro, LUTs (exp de decaimento, seno), macros de saturação, runner de testes, CI local (`make test`, `make asan`, `make bench`).
- Aceite: build limpo com `-Wall -Wextra -Werror`, testes de RNG determinístico e arena.

### FASE 1 — Núcleo SNN (sequencial)

**Lote 1.1 — Neurônios + sinapses + passo (SNN mínima)**
- Arquivos: `snn.*`, `brain.*` (apenas montagem), `tests/test_snn.c`, `bench/bench_snn.c`.
- Entregas: LIF, refratário, CSR com atraso, fila de spikes, construtores de topologia (feedforward, aleatória E/I 80/20, pequeno-mundo).
- Aceite: rede de 5k neurônios roda ≥ 20× tempo real em 1 núcleo; 0 alocações no passo (verificar com contador de arena).

**Lote 1.2 — Plasticidade (juntos, mesmos arquivos)**
- Arquivos: `plasticity.*`, `snn.*` (ganchos), `tests/test_stdp.c`.
- Entregas: STDP pareado por traços, **eligibility trace** (3 fatores), STP Tsodyks–Markram (cobre **ideia 11**), *synaptic scaling* e limiar adaptativo.
- Aceite: teste clássico (par pré→pós potencia, pós→pré deprime); com dopamina=0 não há mudança de peso líquida.

### FASE 2 — Neuromodulação e homeostase (Bloco 2)

**Lote 2.1 — Neuromoduladores + dor reflexa (ideias 7, 8)**
- Arquivos: `neuromod.*`, `brain.*`, `tests/test_neuromod.c`.
- Entregas: variáveis globais com difusão lenta (DA, NE, 5-HT, ACh, cortisol, ocitocina) que modulam ganho, limiar e taxa de aprendizado; **via de reflexo nociceptivo** que liga diretamente sensor de sacudida → motor (bypass da região associativa).
- Aceite: sacudir o mouse dispara motor em ≤ 2 ticks; NE alta aumenta ganho sensorial mensurável.

**Lote 2.2 — CPG + febre cognitiva + energia/sono [P com 2.1 se separar arquivos de `brain.*`]**
- Arquivos: `cpg.*`, `homeostasis.*`.
- Entregas: **CPG** de 8–16 neurônios (**ideia 9**), acelera com cortisol; **febre cognitiva** (**ideia 12**) monitora taxa global e força "apagão" curto; ciclo vigília/sono/REM com *replay* (**ideia 18**, parte do cérebro; o corpo vem na Fase 5).
- Aceite: respiração oscila estável; taxa global saturada ⇒ blackout em < 2 s e retorno ao limiar.

**Lote 2.3 — Erro de predição temporal (ideia 10)**
- Arquivos: `src/core/predict.c/.h`, `brain.*`.
- Entregas: neurônios de expectativa (memória de intervalos de eventos) e *mismatch spike* quando o evento esperado não ocorre.
- Aceite: padrão "clique, clique" aprendido; omissão do 2º gera spike de surpresa e pico de ACh/NE.

### FASE 3 — Plataforma, janela e renderização

**Lote 3.1 — PAL + headless**
- Arquivos: `pal/pal.h`, `pal_headless.c`, `pal_win32.c` e/ou `pal_x11.c`/`pal_macos.m`.
- Interface (esboço):
  ```c
  typedef struct { int x,y; uint32_t buttons; int wheel; uint64_t t_us; } PalMouse;
  typedef struct { int x,y,w,h; uint64_t id; bool focused; } PalWindow;
  bool pal_init(PalConfig*);  void pal_poll(PalEventQueue*);
  int  pal_enum_windows(PalWindow*, int max);
  bool pal_get_caret(int *x,int *y);          // pode retornar false
  float pal_cpu_load(void);   int pal_temp_c(void); // -1 se indisponível
  void pal_present(const uint32_t *argb, int w,int h, int x,int y);
  ```
- Windows: janela `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT` (click-through, alternando quando o mouse está sobre o pet) + `UpdateLayeredWindow` com bitmap ARGB pré-multiplicado.
- Linux/X11: visual ARGB + `XShapeCombineMask` ou composição; **Wayland** restringe posicionamento global e leitura do cursor: documentar limitação e usar XWayland/`layer-shell` quando possível.
- Aceite: janela transparente do pet em 60 fps com < 1% CPU; modo headless para CI.

**Lote 3.2 — Rasterizador e corpo base**
- Arquivos: `body/render.c`, `body/body.c`, `body/palette.c`.
- Entregas: rasterizador de software (elipses antialias, linhas, círculos para olhos), buffer ARGB de ~128×128 a 256×256 e *dirty rect*; corpo como massa-mola de poucos nós.
- Aceite: 60 fps em CPU única; buffer < 256 KB.

### FASE 4 — Sensores do SO (Bloco 1 + parte do Bloco 4) — **todos [P]**

Cada sensor converte um sinal do SO em **população de neurônios de entrada** (codificação por taxa/tempo) com normalização e *rate limiting*.

| Lote | Ideia | Arquivo | Como |
|---|---|---|---|
| 4.1 | Cursor, vetor, velocidade, parada, sacudida, atrito (**24**) | `sense_mouse.c` | Posição em 8×8 + vetor velocidade/aceleração; "sacudida" = reversões de direção por janela de tempo; contato = distância cursor↔corpo + velocidade relativa |
| 4.2 | **Caret tracking (1)** | `sense_caret.c` | Win: `GetGUIThreadInfo`/UIA TextPattern; Linux: AT-SPI. Se indisponível, fallback: cadência de teclas **sem gravar conteúdo** (somente contagem/ritmo) |
| 4.3 | **Vento de rolagem (2)** | `sense_scroll.c` | Acumulador com decaimento de `wheel` → neurônios de "corrente de ar" |
| 4.4 | **Luminosidade local (3)** + **mapa de calor 8×8 (17)** | `sense_luma.c`, `sense_heatmap.c` | Captura **pequena** (ex.: 16×16 px sob o pet) a 2–5 Hz (DXGI/`BitBlt`/`XGetImage`); heatmap = contadores de ocupação por setor com decaimento lento |
| 4.5 | **Carga/temperatura (4)** | `sense_sysload.c` | CPU: `GetSystemTimes`/`/proc/stat`. Temperatura: `/sys/class/thermal` (Linux); no Windows é inconsistente ⇒ usar **carga de CPU como proxy** e tratar temperatura como opcional |
| 4.6 | **Áudio do sistema (5)** | `sense_sysaudio.c` | WASAPI *loopback* / PulseAudio-PipeWire *monitor* via `miniaudio`; extrair só **envelope RMS + 4–8 bandas** (graves por filtro/FFT curta); nunca guardar o áudio |
| 4.7 | **Janelas: topologia (6), foco/Alt+Tab (22), áreas perigosas (23)** | `sense_windows.c` | `EnumWindows` + `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)`; `SetWinEventHook(EVENT_SYSTEM_FOREGROUND)`; botão fechar = canto sup. dir. da janela ativa (zona de risco). Linux: `_NET_CLIENT_LIST`, `_NET_ACTIVE_WINDOW` |

Agrupamento sugerido para sessões: **(4.1 + 4.3)** mouse/scroll; **(4.4 + 4.7)** tela/janelas; **(4.5 + 4.6)** sistema/áudio; **4.2** sozinho (depende de acessibilidade).

Aceite da fase: cada sensor tem teste com **fluxo sintético** (replay) e custo de CPU medido individualmente (< 0,3% cada).

### FASE 5 — Corpo, motor e expressão (Bloco 5) — **[P] entre 5.1/5.2/5.3**

**Lote 5.1 — Elasticidade e pupilas (ideias 25, 28)**
- Arquivos: `body/body.c`, `body/render.c`.
- Entregas: disparos motores → tensão das molas; *squash/stretch* ao longo do vetor de movimento; pupilas independentes (luz fecha, alerta/carinho abre), com `( • _ • )` ↔ `( ◉ _ ◉ )` como estilos de olho.

**Lote 5.2 — Aura cromática (27)**
- Arquivos: `body/palette.c`.
- Entregas: mapa do vetor neuroquímico → matiz/saturação/brilho (frio=apatia/sono, verde-amarelado=calma, quente=excitação/pânico) com suavização temporal.

**Lote 5.3 — Voz procedural (26)**
- Arquivos: `body/voice.c`, `pal` (saída de áudio via `miniaudio`).
- Entregas: oscilador FM controlado por taxa motora (ronronar grave a calma, chiado agudo a pânico); *envelope* ADSR; sem arquivos externos; buffer de 20–40 ms para baixa latência. (O plano 03 estende este módulo para fala aprendida.)

**Lote 5.4 — Espasmos REM (18, parte corporal) + mimetismo rítmico (29)**
- Arquivos: `behavior/rem.c`, `behavior/mimic.c`, `brain.*`.
- Entregas: replay motor durante REM aciona contrações pequenas (pata/orelha); detector de oscilação lenta do mouse (autocorrelação em janela ~4 s) sincroniza CPG e balanço.

### FASE 6 — Comportamento e psicologia (Blocos 3 e 4) — **parcialmente [P]**

Dependem das Fases 2, 4 e 5. Cada comportamento = **pool neural nomeado** + regras de seleção por competição (WTA suave) + efeito motor.

| Lote | Ideias | Arquivo(s) | Mecanismo |
|---|---|---|---|
| 6.1 | **13 DMN** | `behavior/dmn.c` | Ativa quando o ocioso > X s; gera micro-comportamentos aleatórios ponderados (bocejo, limpar patas, olhar ponto) |
| 6.2 | **14 Teoria da mente espacial** | `behavior/anticipate.c` | STDP entre vetor do cursor e pontos de parada (heatmap de destinos); corpo vira antes |
| 6.3 | **15 Apego/saudade** + **16 ressentimento/confiança** | `behavior/bond.c` | Contadores metabólicos lentos: carência sobe com o tempo sem carinho; aversão sobe com sacudidas e decai só com movimentos lentos (juntos: mesmo arquivo/estado) |
| 6.4 | **17 curiosidade por regiões** + **21 território favorito** | `behavior/explore.c` | Heatmap de novidade + recompensa (DA) por locais calmos ⇒ canto favorito emerge por R-STDP |
| 6.5 | **19 esconder-se** + **23 fobia de áreas** + **20 caça** + **22 reação Alt+Tab** | `behavior/hide_hunt.c` | Máquina de estados guiada por pools límbicos (medo vs. energia/bom humor); esconder: posicionar corpo 80% atrás da borda da janela mais próxima |
| 6.6 | **6 janelas como galhos** | `behavior/perch.c` | Colisão com topos de janelas (usa `sense_windows`); gravidade só quando "relaxado" |

Aceite da fase: cada comportamento tem teste de integração com *replay* (ex.: sequência de sacudidas ⇒ aversão sobe ⇒ pet mantém distância por N min simulados).

### FASE 7 — Persistência, sono profundo e genoma

**Lote 7.1 — Persistência**
- Arquivos: `persist.*`, `brain.*`.
- Formato: cabeçalho (`magic`, versão, semente, contagem de neurônios/sinapses, hash), blocos de pesos/limiares/estados neuroquímicos, **CRC32/xxHash**, escrita em arquivo temporário + `rename` atômico, **migrações de versão**.
- Aceite: teste de *fuzz* do carregador (bytes aleatórios nunca causam crash).

**Lote 7.2 — Genoma e filiação (ideia 30)**
- Arquivos: `persist.c` (+ `tools/genome.c`).
- Entregas: exportar apenas os pesos plásticos quantizados (ex.: `int8`) + hiperparâmetros; **mutação gaussiana pequena** e *crossover* opcional; "óvulo" = arquivo `.petgen` pequeno; criação de filhote a partir dele.
- Aceite: filhote inicia com comportamento semelhante ao pai e variações mensuráveis.

### FASE 8 — Otimização e entrega

**Lote 8.1** — Perfil e ajuste: `perf`/VTune/Instruments; SIMD (AVX2/NEON) para decaimento/integração; *tick* adaptativo; compressão de tabelas.
**Lote 8.2** — Robustez: 24 h *soak test*, teste de memória (RSS estável), tratamento de sleep/resume do SO, mudança de monitor/DPI.
**Lote 8.3** — Empacotamento: instalador mínimo, ícone de bandeja, menu (pausar, modo silêncio, resetar, sensores liga/desliga), *auto-start* opcional.

---

## 4. Matriz das 30 ideias

| # | Ideia | Fase/Lote | Módulo |
|---|---|---|---|
| 1 | Caret tracking | 4.2 | `sense_caret` |
| 2 | Vento de rolagem | 4.3 | `sense_scroll` |
| 3 | Luminosidade local | 4.4 | `sense_luma` |
| 4 | Carga/temperatura | 4.5 | `sense_sysload` |
| 5 | Áudio do sistema | 4.6 | `sense_sysaudio` |
| 6 | Janelas como galhos | 4.7 + 6.6 | `sense_windows`, `perch` |
| 7 | Neuromodulação global | 2.1 | `neuromod` |
| 8 | Reflexo nociceptivo | 2.1 | `brain` (via direta) |
| 9 | CPG | 2.2 | `cpg` |
| 10 | Erro de predição temporal | 2.3 | `predict` |
| 11 | Fadiga sináptica | 1.2 | `plasticity` (STP) |
| 12 | Febre cognitiva | 2.2 | `homeostasis` |
| 13 | DMN | 6.1 | `dmn` |
| 14 | Teoria da mente espacial | 6.2 | `anticipate` |
| 15 | Apego/saudade | 6.3 | `bond` |
| 16 | Ressentimento/confiança | 6.3 | `bond` |
| 17 | Regiões inexploradas | 4.4 + 6.4 | `sense_heatmap`, `explore` |
| 18 | REM com espasmos | 2.2 + 5.4 | `homeostasis`, `rem` |
| 19 | Esconder-se | 6.5 | `hide_hunt` |
| 20 | Caça predatória | 6.5 | `hide_hunt` |
| 21 | Território favorito | 6.4 | `explore` |
| 22 | Alt+Tab | 4.7 + 6.5 | `sense_windows` |
| 23 | Fobia de áreas | 4.7 + 6.5 | `sense_windows`, `hide_hunt` |
| 24 | Atrito com ponteiro | 4.1 | `sense_mouse` |
| 25 | Squash & stretch | 5.1 | `body` |
| 26 | Voz procedural | 5.3 | `voice` |
| 27 | Aura cromática | 5.2 | `palette` |
| 28 | Pupilas | 5.1 | `body`, `render` |
| 29 | Mimetismo rítmico | 5.4 | `mimic` |
| 30 | Genoma transgeracional | 7.2 | `persist`, `tools/genome` |

---

## 5. Ordem recomendada (caminho crítico)

```
0.1 → 1.1 → 1.2 → 2.1 ──┬→ 2.2 ──┐
                        └→ 2.3 ──┤
3.1 → 3.2 ───────────────────────┼→ 5.1/5.2/5.3 → 6.x → 7.1 → 7.2 → 8.x
        4.1..4.7 (paralelo) ─────┘
```

Primeiro marco jogável (**MVP**): `0.1 → 1.1 → 1.2 → 2.1 → 3.1 → 3.2 → 4.1 → 5.1`. Resultado: pet na tela, olhando e reagindo ao mouse com cérebro SNN real.

---

## 6. Testes e qualidade

- **Unitários** por módulo (neurônio, STDP, STP, CPG, neuromoduladores).
- **Determinismo**: mesma semente + mesmo *replay* ⇒ *hash* idêntico do estado após N ticks.
- **Propriedade**: pesos nunca saem de `[wmin, wmax]`; taxa global nunca fica presa no teto por > T (febre cognitiva funciona).
- **Fuzz** do carregador de cérebro e do genoma.
- **Soak 24 h** com *replay* acelerado; RSS estável.
- **Benchmarks** versionados em `bench/` (neurônios/s, spikes/s, µs por tick); falha de CI se regressão > 10%.
- Compilação: `-std=c11 -O2 -Wall -Wextra -Werror`, perfis `asan`, `ubsan`, `tsan` (para filas entre threads).

## 7. Riscos e mitigações

| Risco | Mitigação |
|---|---|
| Instabilidade da rede (epilepsia/silêncio) | Balanço E/I, scaling, limiar adaptativo, febre cognitiva, testes de propriedade |
| Wayland/macOS limitam overlay e leitura global | PAL com capacidades declaradas; degradar graciosamente |
| Temperatura de hardware indisponível | Usar carga de CPU como proxy |
| Leitura de texto/teclado sensível | Nunca registrar teclas; acessibilidade com bloqueio de campos de senha; lista de apps ignorados |
| Crescimento de memória | Arenas fixas + teste de RSS + limites rígidos de tabelas |
| Antivírus marcando *hooks* globais | Preferir *polling* e APIs de eventos oficiais; evitar *keyboard hooks* |

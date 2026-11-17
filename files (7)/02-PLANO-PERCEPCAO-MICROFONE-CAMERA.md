# 02 — Plano: Percepção por Microfone e Câmera

Objetivo: quando o usuário **ligar** o microfone e/ou a câmera, o pet passa a **ouvir e ver** o ambiente e **reage** ao que percebe. Quando estiverem desligados, o módulo fica totalmente inativo (zero threads, zero buffers, zero CPU).

Pré-requisitos: Fases 0–3 do plano 01 (núcleo, neuromoduladores, PAL, corpo). Este plano **alimenta** o plano 03 (linguagem) com unidades de áudio e eventos visuais.

---

## 1. Princípios

1. **Consentimento explícito e reversível**: dois interruptores independentes (mic, câmera), desligados por padrão, no menu da bandeja.
2. **Indicador visível sempre**: ícone/olho do pet muda (ex.: "orelha em pé" para mic, "olhos arregalados" para câmera) e um ponto na bandeja. Nunca captura oculta.
3. **100% local**: nenhum dado sai da máquina. Sem rede.
4. **Sem gravação**: áudio e vídeo bruto vivem apenas em **buffers circulares pequenos na RAM** e são sobrescritos. Só são persistidos **características numéricas abstratas** (vetores/estatísticas) e, no plano 03, tabelas de unidades aprendidas.
5. **Degradação suave**: sem dispositivo, sem permissão ou sem suporte ⇒ módulo desativa e o pet continua normal.
6. **Orçamento**: mic ≤ +1% CPU e ≤ +3 MB RAM; câmera ≤ +2% CPU e ≤ +4 MB RAM.

---

## 2. Arquitetura

```
 ┌────────────┐   PCM 16 kHz mono    ┌──────────────┐   spikes    ┌─────────┐
 │ miniaudio  │ ───────────────────▶ │ audio_feat   │ ──────────▶ │         │
 │ (mic)      │                      │ (ENV,bandas, │             │         │
 └────────────┘                      │  MFCC, VAD)  │             │  CÉREBRO│
                                     └──────────────┘             │  (SNN)  │
 ┌────────────┐   frames 96x72 gray  ┌──────────────┐   spikes    │         │
 │ câmera     │ ───────────────────▶ │ vision_feat  │ ──────────▶ │         │
 │ (MF/V4L2)  │                      │ (luz,mov,    │             │         │
 └────────────┘                      │  fluxo,rosto)│             └─────────┘
                                     └──────────────┘
```

Estrutura de arquivos nova:

```
src/sense/
├─ audio_in.c         # captura (miniaudio), buffer circular, thread própria
├─ audio_feat.c       # envelope, bandas, MFCC, VAD, localização grosseira
├─ vision_in.c        # captura câmera (PAL), downscale
├─ vision_feat.c      # luminância, movimento, fluxo óptico, rosto, "presença"
├─ percept_events.c   # eventos de alto nível (palma, voz, porta, presença)
└─ consent.c          # estado de consentimento + indicadores + persistência
src/pal/
├─ pal_cam_win32.c    # Media Foundation
├─ pal_cam_v4l2.c     # Linux
└─ pal_cam_macos.m    # AVFoundation
tests/test_audio_feat.c, test_vision_feat.c, test_percept_events.c
```

Threads: **1 thread de captura** por dispositivo (prioridade baixa) → escreve em buffer circular → o *tick* do cérebro consome características já reduzidas (≈ 50 Hz áudio, 5–10 Hz visão). A thread principal nunca bloqueia em E/S.

---

## 3. Etapas

### ETAPA A — Consentimento, indicadores e PAL de dispositivos (juntas)
- Arquivos: `consent.c`, `pal.h` (adicionar `pal_audio_open/close`, `pal_cam_open/close`, `pal_has_mic/cam`), menu da bandeja.
- Entregas: interruptores, estados persistidos, indicadores no corpo, enumeração de dispositivos, tratamento de "permissão negada".
- Aceite: com ambos desligados, **nenhuma** thread de captura existe (verificado por teste).

### ETAPA B — Áudio: captura e características básicas (B1 + B2 juntas: mesmos arquivos)
- Arquivos: `audio_in.c`, `audio_feat.c`.
- **B1 Captura**: `miniaudio` em modo *capture*, 16 kHz mono `int16`, *callback* escreve em *ring buffer* de ~2 s (64 KB).
- **B2 Características** por quadro de 20–25 ms, passo 10 ms:
  - **RMS/envelope** (volume) e **taxa de subida** (detecta sustos/palmas/batidas).
  - **Bandas** (ex.: 8 bandas Mel) via `pffft`/`kissfft` com janela de 512.
  - **Fluxo espectral** (mudança entre quadros) ⇒ novidade sonora.
  - **Zero-crossing rate** e **centroide espectral** (voz vs. ruído).
  - **Piso de ruído adaptativo** (média móvel de mínimos) para calibrar sala silenciosa vs. barulhenta.
- Codificação em spikes: população tonotópica (cada banda → grupo de neurônios; taxa ∝ energia normalizada); neurônios de **onset** (derivada positiva) e de **offset**.
- Aceite: palmas geram *onset spike* em ≤ 20 ms; sala silenciosa ⇒ atividade quase nula.

### ETAPA C — Áudio: eventos de alto nível (C1 + C2)
- Arquivos: `audio_feat.c`, `percept_events.c`.
- **C1 VAD** (detecção de atividade de voz) em duas camadas: energia + ZCR + centroide (barata); opcionalmente uma rede minúscula treinada offline e exportada como `.h` de pesos (se for preciso mais robustez).
- **C2 Classificador grosseiro de eventos** por regras + pequena SNN: `silêncio | ruído contínuo | impulso (palma/batida) | voz | música-ritmo`. Para **ritmo**: autocorrelação do envelope em janela de 4–6 s ⇒ BPM aproximado, alimenta mimetismo rítmico (ideia 29 do plano 01).
- **Localização grosseira** (só com microfone estéreo): diferença interaural de nível/tempo ⇒ esquerda/centro/direita, o pet vira a cabeça.
- Reações-alvo:
  - Impulso forte súbito ⇒ **sobressalto** (reflexo, NE↑, cortisol↑ leve).
  - Voz do usuário (calma) ⇒ orelhas/olhos atentos, ocitocina↑ se o tom for suave e ritmo lento.
  - Voz alta/áspera repetida ⇒ medo/ressentimento (liga em 6.3 do plano 01).
  - Música ritmada ⇒ balanço sincronizado.
- Aceite: testes com arquivos de áudio sintéticos (tons, impulsos, ruído branco, "voz" simulada por pulsos harmônicos) geram os eventos esperados; taxa de falso positivo < 2% em 10 min de ruído de sala.

### ETAPA D — Visão: captura e características de baixo nível (D1 + D2 juntas)
- Arquivos: `vision_in.c`, `vision_feat.c`, `pal_cam_*.c`.
- **D1 Captura**: pedir 160×120 ou 320×240 a 10–15 fps ao driver; converter para **cinza 96×72** (ou 64×48 para economizar mais), descartar o frame colorido imediatamente.
- **D2 Características**:
  - **Luminosidade média** (dia/noite, luz acesa/apagada) ⇒ pupilas e ritmo circadiano (alimenta sono).
  - **Diferença de quadros** em grade 8×6 ⇒ **mapa de movimento** (quanto e onde).
  - **Fluxo óptico esparso** (Lucas–Kanade em ~32 pontos) ⇒ direção/velocidade dominante.
  - **Presença** (movimento + estabilidade de fundo): "tem alguém na frente" vs. "sala vazia".
- Codificação: grade 8×6 de neurônios retinotópicos (taxa ∝ movimento) + neurônios de direção (4–8 direções) + neurônio de luz global.
- Aceite: mão acenando gera atividade no setor correto; sala vazia estável ⇒ sem disparos espúrios (piso adaptativo de ruído do sensor).

### ETAPA E — Visão: eventos de alto nível (E1 + E2)
- Arquivos: `vision_feat.c`, `percept_events.c`.
- **E1 Rosto/olhar (opcional)**: detector leve (candidato: cascata tipo `pico`, em C; **verificar licença**) a 3–5 Hz apenas quando há movimento. Saídas: posição/tamanho do rosto. O pet **olha para o rosto** do usuário e acompanha.
- **E2 Eventos**: `entrou alguém | saiu | aproximou-se | afastou-se | gesto de aceno | movimento brusco (susto visual) | luz acendeu/apagou`.
- Reações-alvo:
  - Luz apaga ⇒ pupila dilata, bocejo, sonolência (liga ao ciclo de sono).
  - Movimento brusco perto da câmera ⇒ sobressalto.
  - Aceno lento ⇒ curiosidade + aproximação ao lado da câmera/tela.
  - Usuário some por muito tempo ⇒ saudade (liga a 6.3).
- Aceite: testes com sequências de quadros sintéticos (retângulos em movimento, mudança global de brilho).

### ETAPA F — Integração multimodal (F1 + F2)
- Arquivos: `percept_events.c`, `brain.c`, `behavior/*`.
- **F1 Detector de coincidência audiovisual**: neurônios que só disparam quando som e movimento ocorrem juntos (janela de ~150 ms) ⇒ "isso foi uma ação" (palma vista e ouvida), reforça saliência (↑NE/DA).
- **F2 Modulação cruzada**: áudio alto + escuro ⇒ medo maior; voz suave + presença próxima ⇒ confiança.
- Também entram os sensores do plano 01 (mouse/janelas) no mesmo espaço de saliência para que o pet **escolha o que atender** (competição por atenção; ver "espaço de trabalho global" no plano 03).
- Aceite: pet atende ao estímulo mais saliente; ignora o ruído contínuo (habituação por STP).

### ETAPA G — Habituação, privacidade e robustez
- **Habituação**: estímulos repetidos perdem efeito (STP/fadiga), novidade o recupera.
- **Privacidade (testes obrigatórios)**:
  - Nenhum arquivo de áudio/vídeo bruto é criado (teste que monitora gravações em disco durante o run).
  - Memória: buffers brutos zerados ao desligar o interruptor.
  - O arquivo de cérebro contém apenas pesos/estatísticas (revisão do formato).
- **Robustez**: dispositivo removido a quente, mudança de formato, sleep/resume, dois apps disputando a câmera (liberar e tentar de novo com *backoff*).
- Aceite: 8 h com mic/câmera ligados sem vazamento de memória; CPU dentro do orçamento.

---

## 4. Parâmetros iniciais sugeridos

| Parâmetro | Valor |
|---|---|
| Áudio | 16 kHz, mono, `int16`, quadro 25 ms / passo 10 ms, 8 bandas Mel |
| Ring buffer de áudio | 2 s (64 KB) |
| Vídeo | 160×120 capturado → 96×72 cinza, 10 fps (5 fps em repouso) |
| Grade retinotópica | 8×6 (48 neurônios de movimento) + 8 direções + 1 luz |
| Janela de coincidência AV | 150 ms |
| Habituação (τ de recuperação) | 20–60 s |
| Limite de CPU (watchdog) | se exceder 2× o orçamento por > 10 s, reduzir taxa (fps/quadros) automaticamente |

## 5. Ordem e paralelismo

```
A ─┬─▶ B ─▶ C ──┐
   └─▶ D ─▶ E ──┼─▶ F ─▶ G
```
B/C (áudio) e D/E (visão) são **independentes** e podem ser feitos em paralelo após A.

## 6. Testes

- **Fixtures sintéticas** (geradas por código): tons, impulsos, ruído, sequências de quadros; nenhum arquivo real de pessoa é necessário.
- **Determinismo**: com entrada gravada (características, não áudio bruto) o comportamento é reproduzível.
- **Carga**: medir CPU/RAM com cada sensor ligado separadamente e juntos.
- **Segurança**: *fuzz* nos parsers de formatos de captura e nas rotinas de conversão de quadros.

<div align="center">

# 🐾 Caddin — Organismo Virtual Sintético v1.4.0
### Mascote Autônomo para Desktop com SNN de 2048 Neurônios, Ontogenia Biológica, Memória Hiperdimensional SDM, Bandit Dopaminérgico UCB1, Percepção Multimodal e Telemetria em C11 Puro

[![Linguagem](https://img.shields.io/badge/Linguagem-C11%20Puro%20(Zero%20Python)-blue.svg)](#)
[![RAM](https://img.shields.io/badge/Consumo%20RAM-~12%20MB-brightgreen.svg)](#)
[![Alocação](https://img.shields.io/badge/Alocação-Zero%20Malloc%20no%20Loop-orange.svg)](#)
[![Aritmética](https://img.shields.io/badge/Aritmética-Ponto%20Fixo%20Q8.8%20%2F%20Q0.32-purple.svg)](#)
[![Testes](https://img.shields.io/badge/Testes-13%2F13%20Aprovados%20(100%25)-success.svg)](#)
[![Plataformas](https://img.shields.io/badge/Plataformas-Linux%20(X11%20%2B%20ALSA)%20%7C%20Windows%20(GDI%20%2B%20WinMM)-blueviolet.svg)](#)
[![Licença](https://img.shields.io/badge/Licença-MIT%20Open%20Experimental-lightgrey.svg)](#)

</div>

---

## 📖 Apresentação

O **Caddin** é um companheiro virtual vivo para desktop projetado como uma **simulação neurobiológica contínua e autônoma** em tempo real a 60 FPS. Diferente de mascotes convencionais que executam scripts de animação pré-renderizados ou agentes LLM em nuvem, o Caddin é um **organismo sintético guiado por disparos de potenciais de ação (spikes)** em uma rede neural de 2048 neurônios biológicos acoplada à dinâmica de 5 neuromoduladores endógenos, física elástica de corpo mole deformável (softbody), audição procedural, visão contextual do fluxo de trabalho do usuário e ontogenia com maturação corporal e vocal.

Desenvolvido estritamente em **C11 nativo sem alocação dinâmica no laço principal** e com aritmética de ponto fixo determinística (`fix16_t` Q8.8 e `brake` Q0.32), o programa consome apenas **~12 MB de memória RAM**, rodando com fluidez sem necessidade de GPU dedicada, OpenGL, Mesa, LLVM ou bibliotecas pesadas.

---

## 📑 Índice Geral

1. [Início Rápido e Execução](#-início-rápido-e-execução)
2. [Matriz Completa de Capacidades do Organismo](#-matriz-completa-de-capacidades-do-organismo)
3. [Arquitetura Neurobiológica em 4 Camadas](#-arquitetura-neurobiológica-em-4-camadas)
4. [Cérebro SNN de 2048 Neurônios e Topologia Neural](#-cérebro-snn-de-2048-neurônios-e-topologia-neural)
5. [Ciclo Ontogenético e Janela Crítica de Aprendizado](#-ciclo-ontogenético-e-janela-crítica-de-aprendizado)
6. [Memória Episódica SDM (Kanerva) & Curiosidade (Schmidhuber)](#-memória-episódica-sdm-kanerva--curiosidade-schmidhuber)
7. [Bandit Dopaminérgico UCB1 & Detecção de Fadiga Humana](#-bandit-dopaminérgico-ucb1--detecção-de-fadiga-humana)
8. [Maturação Vocal & Síntese Procedural Acústica (11 Tipos)](#-maturação-vocal--síntese-procedural-acústica)
9. [Física Elástica Softbody e Reações Mecânicas](#-física-elástica-softbody-e-reações-mecânicas)
10. [Percepção Contextual de Janelas, Código e Privacidade](#-percepção-contextual-de-janelas-código-e-privacidade)
11. [Rituais Circadianos e Marcos de Convivência](#-rituais-circadianos-e-marcos-de-convivência)
12. [Biometria de Digitação (IKD) & Resumo Diário de Foco](#-biometria-de-digitação-ikd--resumo-diário-de-foco)
13. [Monitor de Telemetria e Spikes em Terminal ANSI em 100% C](#-monitor-de-telemetria-e-spikes-em-terminal-ansi)
14. [Motor de Regras Customizáveis (.caddinrc)](#-motor-de-regras-customizáveis-caddinrc)
15. [Persistência Atômica v2 com Integridade CRC32](#-persistência-atômica-v2-com-integridade-crc32)
16. [Estrutura de Diretórios do Projeto](#-estrutura-de-diretórios-do-projeto)
17. [Histórico de Versões e Evolução](#-histórico-de-versões-e-evolução)
18. [Guia de Compilação, Cross-Compile e Deploy](#-guia-de-compilação-cross-compile-e-deploy)
19. [Suíte de Testes Automatizada (13/13 Aprovadas)](#-suíte-de-testes-automatizada)
20. [Controles, Interações e Atalhos](#-controles-interações-e-atalhos)
21. [Licença](#-licença)

---

## 🚀 Início Rápido e Execução

### Linux (Nativo X11 + ALSA)
```bash
# 1. Instalar dependências básicas de compilação
sudo apt install gcc make cmake libx11-dev libxext-dev libasound2-dev

# 2. Clonar o repositório
git clone https://github.com/HololiveStudios/caddin.git
cd caddin

# 3. Compilar em modo Release
make

# 4. Executar a suíte completa de testes unitários (13/13)
make test

# 5. Iniciar o mascote
make run
```

### Monitor de Telemetria em Tempo Real (Terminal Separado, 100% C Puro)
```bash
# Permite assistir a atividade dos 2048 neurônios, ondas de neuromoduladores e fadiga
./build/caddin_monitor
```

### Windows (Compilação Cruzada via MinGW-w64 no Linux)
```bash
sudo apt install gcc-mingw-w64-x86-64 ninja-build cmake
make windows
# Binários gerados: build-windows/caddin.exe e build-windows/caddin_monitor.exe
```

---

## 🧬 Matriz Completa de Capacidades do Organismo

| Dimensão | Componente Interno | Funcionamento e Especificação Técnica |
|---|---|---|
| **Cérebro Spiking** | 2048 Neurônios LIF | 64 canais sensoriais (512 neurônios), 32 pools associativos (1152 neurônios) e 24 motores (384 neurônios). Sinapses CSR esparsas com R-STDP e STP de Tsodyks-Markram. |
| **Ontogenia Biológica** | 120 Pools Ontogenéticos | Maturação em 4 estágios (*Filhote, Juvenil, Adulto, Veterano*). Freio da janela crítica com aritmética Q0.32 e cristalização de projeções estáveis para `int8`. |
| **Memória Episódica** | SDM Hiperdimensional | Sparse Distributed Memory de Kanerva com hipervetores de 2048 bits ($32 \times \text{uint64\_t}$). Conclusão de padrão resistente a ruído e poda sináptica no sono. |
| **Curiosidade Intrínseca** | Princípio de Schmidhuber | Injeção de dopamina por progresso de compressão temporal ($\Delta \text{err} < -2$). Elevação do drive de exploração quando o ambiente está estagnado. |
| **Inteligência Contextual** | Bandit UCB1 | Exploração vs. exploração de intervenções ergonômicas baseadas no perfil de fadiga e aceitação implícita do usuário. |
| **Visão & Desktop** | Percepção de Texto X11/Win32 | Monitoramento em tempo real do contexto de tela com detecção nativa de erros (`segfault`, `panic`) e sucessos de compilação (`passed`, `build completed`). |
| **Audição Acústica** | Captura de Microfone ALSA | Processamento RMS não-bloqueante; reage a sons súbitos, palmas e intensidade da fala humana. |
| **Biometria de Teclado** | Keystroke Biometrics IKD | Reconhece o dono pela cadência de digitação (Inter-Keystroke Delay em 16 bins), elevando serotonina com o dono e noradrenalina com estranhos. |
| **Física Softbody** | Dinâmica Elástica 2D | Equação massa-mola amortecida contínua com deformações de *Squish*, *Stretch*, *Shear* e expansão respiratória acoplada ao CPG. |
| **Voz Sintética** | Modelagem Acústica 16 kHz | 11 tipos de vocalizações sintetizadas proceduralmente (sem áudios gravados). Frequências fundamentais, harmônicos, vibrato, tremolo e ruído. |
| **Telemetria ANSI** | FIFO POSIX Não-Bloqueante | Emite telemetria binária a 10 Hz para visualização em terminal C puro (`caddin_monitor`). |
| **Resumo Diário** | Daily Summary Logger | Mantém métricas diárias agregadas de convivência, código e foco em `caddin_daily.log`. |
| **Privacidade Estrita** | Blacklist de Segurança | Ignora qualquer leitura de janela pertencente a gerenciadores de senhas (`bitwarden`, `keepass`, `1password`), bancos e sessões privadas. |
| **Personalização** | Parser `.caddinrc` | Configuração de regras ativas por nome de janela, horários de foco, injeção direta de neuromoduladores e ações físicas. |

---

## 🏛️ Arquitetura Neurobiológica em 4 Camadas

```mermaid
flowchart TD
    subgraph C1["CAMADA 1: Percepção Multimodal do Sistema Operacional"]
        M1["Mouse: Posição, Atrito, Sacudida, Jitter"]
        M2["Teclado: Cadência, Pausas, Biometria IKD"]
        M3["Tela: Títulos Ativos, Erros e Sucessos de Build"]
        M4["Áudio: Microfone Ambiente RMS"]
        M5["Hardware: Carga de CPU, Memória, Temperatura"]
    end

    subgraph C2["CAMADA 2: Cérebro SNN (2048 Neurônios) & CPG"]
        S1["512 Sensoriais (64 Pools de Entrada)"]
        S2["1152 Associativos Límbicos e Cognitivos"]
        S3["384 Motores (Vocalizações, Expressões, Movimento)"]
        CPG["Central Pattern Generator (Respiração & Ritmo)"]
    end

    subgraph C3["CAMADA 3: Neuromodulação, Memória & Ontogenia"]
        N1["Neuromoduladores: DA, NE, SER, ACh, CORT"]
        N2["Ontogenia: Janela Crítica & Freio Inibitório Q0.32"]
        N3["Memória Episódica SDM (Hipervetores 2048 bits)"]
        N4["Bandit Dopaminérgico UCB1 (Intervenções Inteligentes)"]
        N5["Ring Buffer de Causalidade (256 Eventos Recentes)"]
    end

    subgraph C4["CAMADA 4: Atuação Física, Vocal & Comportamental"]
        A1["Softbody Físico 2D (Squish, Stretch, Jiggle)"]
        A2["Síntese de Voz Procedural 16 kHz (11 Tipos)"]
        A3["Árvore de Comportamentos: Perch, Hide, Hunt, Dance"]
        A4["Renderizador Pixel-Art com Olhos Expressivos"]
        A5["FIFO de Telemetria e Logger Diário"]
    end

    C1 --> C2
    C2 <--> C3
    C2 --> C4
    C3 --> C4
```

---

## 🧠 Cérebro SNN de 2048 Neurônios e Topologia Neural

A rede opera a 60 quadros por segundo com neurônios **LIF (Leaky Integrate-and-Fire)** e sinapses esparsas em formato **CSR (Compressed Sparse Row)**:

### 1. Canais Sensoriais de Entrada (64 Pools × 8 Neurônios = 512)
- **Digitação & Ritmo:** `TYPING`, `KEY_FAST`, `KEY_SLOW`, `IDLE`, `PAUSE`, `CMD_ENTER`, `BACKSPACE`, `DELETE`.
- **Mouse & Físico:** `MOUSE_MOVE`, `MOUSE_FAST`, `MOUSE_SHAKE`, `MOUSE_DRAG`, `CLICK_LEFT`, `CLICK_RIGHT`, `WHEEL_UP`, `WHEEL_DOWN`.
- **Janelas & Desktop:** `WIN_ACTIVE`, `WIN_SWITCH`, `WIN_MINIMIZE`, `WIN_MAXIMIZE`, `WIN_MOVE`, `WIN_RESIZE`, `WIN_CLOSE_DANGER`.
- **Hardware & Sistema:** `CPU_HIGH`, `CPU_NORMAL`, `CPU_IDLE`, `TEMP_HOT`, `TEMP_WARM`, `RAM_HEAVY`, `IO_WAIT`.
- **Ambiente Físico:** `LUMA_DARK`, `LUMA_BRIGHT`, `HEATMAP_DENSE`, `SOUND_LOUD`, `SOUND_QUIET`, `TEXT_NEW_WORD`.

### 2. Interneurônios Associativos (32 Pools × 36 Neurônios = 1152)
Pools de associação límbica, memória de trabalho e plasticidade:
- `EMOTION_REG`, `ALERT`, `PLAYFUL`, `RESTING`, `SURPRISED`, `HYPERACTIVE`, `FEAR_RESPONSE`, `REWARD_EXPECT`, `THREAT_DETECT`, `RHYTHM_GEN`, `FOCUS_POOL`, `COGNITIVE_HIGH`, `COGNITIVE_LOW`, `COMFORT_POOL`, `ENERGY_CONSERVE`, `HABITUATION_POOL`.

### 3. Motores Comportamentais (24 Pools × 16 Neurônios = 384)
- **Expressão Facial:** `M_FACE_HAPPY`, `M_FACE_SAD`, `M_FACE_SLEEP`, `M_FACE_SURPRISED`, `M_FACE_ANGRY`, `M_FACE_CURIOUS`, `M_FACE_PLAYFUL`, `M_FACE_NEUTRAL`, `M_FACE_WINK`.
- **Atuação Vocal:** `M_CHIRP_SOUND`, `M_BEEP_PULSE`, `M_PURR_EFFECT`, `M_ALERT_EMIT`, `M_THINK_EMIT`.
- **Locomoção & Dinâmica:** `M_WANDER_LEFT`, `M_WANDER_RIGHT`, `M_JUMP_ACTION`, `M_DANCE_ACTION`, `M_SLEEP_ACTION`, `M_EAT_ACTION`, `M_NOD_HEAD`, `M_SHAKE_HEAD`, `M_TAIL_WAG`.

### 4. Neuromoduladores Biológicos
- **Dopamina (DA):** Sinal de recompensa, novidade e incentivo à exploração.
- **Noradrenalina (NE):** Sinal de alerta, foco súbito, surpresa e estresse agudo.
- **Serotonina (SER):** Saciedade, bem-estar, afeto consolidado e alívio pós-choque.
- **Acetilcolina (ACh):** Atenção seletiva, foco no trabalho e gravação de memória.
- **Cortisol (CORT):** Estresse crônico, fadiga e abandono prolongado.

---

## 🐣 Ciclo Ontogenético e Janela Crítica de Aprendizado

O módulo de ontogenia (`src/core/ontogeny.c/h`) opera com aritmética de ponto fixo `fix16_t` (Q8.8) e inteiros de 32 bits:

### 1. Freio Inibitório da Janela Crítica (`brake` Q0.32)
Para cada um dos 120 pools biológicos:
- A cada segundo de vigília, se o pool apresentar atividade mínima (`act_ema >= act_min`):
  - **Domínio de Padrões (`err_ema < theta_lo`):** O freio cresce proporcionalmente à precisão preditiva:
    $$\Delta = \left(\frac{2^{32}}{T}\right) \cdot m \cdot \text{act\_ema}$$
  - **Surpresa Crônica (`err_ema > theta_hi`):** O freio regride (reabrindo a plasticidade) em velocidade 4× mais lenta que o fechamento.
- **Taxa de Aprendizado Efetiva:**
  $$\text{lr} = \max(\text{piso}, 1 - \text{brake})$$
  *(Filhotes aprendem a 100%; veteranos retêm ~5% de piso para adaptações sutis).*

### 2. Estágios do Desenvolvimento
| Estágio | Maturidade Média | Comportamento e Fisiologia |
|---|---|---|
| **Filhote (`ONTO_FILHOTE`)** | $0\% \le \text{freio} < 25\%$ | Hiperativo, voz aguda ($1.35\times$), corpo elástico e macio, curiosidade elevada. |
| **Juvenil (`ONTO_JUVENIL`)** | $25\% \le \text{freio} < 60\%$ | Voz intermediária ($1.15\times$), formação de hábitos, consolidação no sono. |
| **Adulto (`ONTO_ADULTO`)** | $60\% \le \text{freio} < 90\%$ | Voz balanceada ($1.00\times$), temperamento firme, afeto consolidado ao dono. |
| **Veterano (`ONTO_VETERANO`)** | $90\% \le \text{freio} \le 100\%$ | Voz profunda ($0.88\times$), alta fidelidade, projeções cristalizadas em `int8`. |

---

## 💾 Memória Episódica SDM (Kanerva) & Curiosidade (Schmidhuber)

### 1. Sparse Distributed Memory (Hipervetores de 2048 Bits)
Implementada em `src/core/sdm.c/h`:
- **Álgebra de Hipervetores:** Operador Bind ($\otimes$) via XOR bit a bit em blocos de 64 bits (`uint64_t`) e Bundle ($\oplus$) via votação majoritária com contadores saturantes.
- **Conclusão de Padrão (Pattern Completion):** 128 hard locations distribuídas. O mascote recorda episódios passados mesmo com ruídos de até 15% nos estímulos de entrada.
- **Poda Sináptica Noturna:** Memórias registradas apenas uma vez decaem a zero durante o sono diário, liberando espaço para novos episódios.

### 2. Curiosidade Guiada por Progresso de Compressão
Baseado na teoria de Jürgen Schmidhuber:
$$\text{Progresso} = \text{Erro}_{t-1} - \text{Erro}_t$$
- **Aprendizado Ativo ($\Delta \text{err} < -2$):** Injeta Dopamina intrínseca por ter decodificado o ambiente.
- **Tédio por Estagnação ($\Delta \text{err} \approx 0$ com erro baixo):** O ambiente ficou previsível demais; eleva a curiosidade para estimular a exploração territorial.

---

## 🎰 Bandit Dopaminérgico UCB1 & Detecção de Fadiga Humana

### 1. Estimador de Fadiga do Usuário (`src/sense/sense_fatigue.c/h`)
Calcula a exaustão física e cognitiva baseando-se em:
- Horas contínuas de tela sem pausas.
- Cadência de digitação (keystrokes por segundo).
- Variância do intervalo entre teclas (irregularidade motora).
- Jitter e micro-tremores na velocidade do mouse.

### 2. Algoritmo UCB1 e Ações Empáticas
Quando a fadiga ultrapassa o limiar e respeitado um cooldown de 5 minutos, o mascote executa:
- `ARM_SILENT`: Observação discreta sem perturbação.
- `ARM_SUGGEST_PAUSE`: Deita-se e emite ronrom suave sugerindo descanso mútuo.
- `ARM_CHEER_PLAY`: Dá um pulo com som alegre de chirp para descontrair a mente.
- `ARM_STRETCH_WATER`: Espreguiça-se com jiggle elástico convidando a beber água.
- **Feedback Implícito:** Carinho após intervenção = $+1.0$; sacudida violenta = $-1.0$.

---

## 🎙️ Maturação Vocal & Síntese Procedural Acústica

A geração de som do Caddin opera a **16.000 Hz sem arquivos WAV ou MP3 em disco**. O áudio é sintetizado em tempo real com ondas senoidais, ruído branco (xorshift32), modulações de amplitude (AM) e envelopes biológicos ADSR.

| Identificador | Significado Biológico | Estrutura Acústica da Onda |
|---|---|---|
| `VOICE_PURR_CONTENT` | Carinho moderado | Onda de 68 Hz com AM triangular de 20 Hz e sopro suave. |
| `VOICE_PURR_DEEP` | Afeto prolongado / relaxamento | Onda fundamental de 55 Hz rica em 3 harmônicos e vibrato profundo. |
| `VOICE_TRILL` | Alegria efervescente / comemoração | Sequência de 3 chirps rápidos ascendentes (450→780 Hz). |
| `VOICE_CHIRP_HAPPY` | Resposta a toque gentil | Duas notas harmônicas (500 Hz e 660 Hz) com decaimento suave. |
| `VOICE_CHIRP_CURIOUS` | Curiosidade e inspeção | Sweep ascendente 380→760 Hz com vibrato progressivo. |
| `VOICE_GRUNT_IDLE` | Bocejo ou alívio | Sweep descendente 320→160 Hz com rouquidão gutural. |
| `VOICE_WHINE_PROTEST` | Saudade ou protesto suave | Descida tonal arrastada com vibrato melancólico. |
| `VOICE_GROWL_ANNOYED` | Irritação ou incômodo | Frequência de 160 Hz com harmônicos inarmônicos felinos. |
| `VOICE_HISS_ANGRY` | Raiva extrema / agressão | Ruído branco puro estocástico com envelope quadrático. |
| `VOICE_SQUEAK_STARTLED` | Susto agudo / choque súbito | Queda rápida 1300→550 Hz sem ataque para expressar sobressalto. |
| `VOICE_CRY_DISTRESS` | Angústia por solidão crônica | Vibrato profundo de $\pm 40$ Hz com soluços respiratórios. |

---

## 🍮 Física Elástica Softbody e Reações Mecânicas

O corpo do Caddin é modelado por um sistema contínuo de osciladores harmônicos amortecidos em 2D:

$$m \frac{d^2 x}{dt^2} + c \frac{dx}{dt} + k (x - x_0) = F_{\text{inércia}} + F_{\text{impacto}} + F_{\text{respiração}}$$

- **Squish (Compressão Vertical):** Amortece quedas e colisões contra as bordas da tela.
- **Stretch (Alongamento Vertical):** Ativado em impulsos de salto ou ao ser erguido pelo mouse.
- **Shear / Jiggle (Deformação Lateral):** Responde ao arrasto inercial do mouse e vibrações comemorativas.
- **Acoplamento Respiratório com CPG:** O Central Pattern Generator expande sutilmente o perímetro do mascote a cada ciclo respiratório.

---

## 👁️ Percepção Contextual de Janelas, Código e Privacidade

O sensor de texto (`src/sense/sense_text.c/h`) examina a janela do sistema operacional em foco:
- **Detecção de Erros de Código:** Reconhece `"error"`, `"failed"`, `"segfault"`, `"panic"`, `"exception"`. Causa injeção de noradrenalina/cortisol, achatamento corporal (squish) e som de resmungo (`VOICE_WHINE_PROTEST`).
- **Detecção de Sucessos de Build:** Reconhece `"success"`, `"passed"`, `"100%"`, `"completed"`. Gera pulso de dopamina/serotonina, salto com jiggle e trilo de alegria (`VOICE_TRILL`).
- **Lista Estrita de Privacidade:** Ignora títulos de gerenciadores de senhas (`bitwarden`, `keepass`, `1password`), janelas anônimas e termos bancários.

---

## 🌅 Rituais Circadianos e Marcos de Convivência

Gerenciados em `src/core/rituals.c/h`:
- **Saudações Circadianas:** Saudação afetuosa matinal entre 6h e 10h; recolhimento noturno suave após as 22h.
- **Marcos Históricos de Vigília:** Celebra marcos de convivência de **1 hora**, **10 horas**, **50 horas** e **100 horas** de vida acordado com dança comemorativa e trilos vocais.

---

## ⌨️ Biometria de Digitação (IKD) & Resumo Diário de Foco

- **Imprinting do Ritmo de Digitação (`src/sense/sense_keystroke_biometrics.c/h`):** Mede o atraso entre teclas (IKD) em 16 bins. Consolida o perfil do dono no sono (`caddin.keys`). Se a cadência for do dono (similaridade $> 0.85$), induz serotonina e afeto; se for de um estranho, induz alerta.
- **Resumo Diário de Produtividade (`src/core/daily_summary.c/h`):** Registra em `caddin_daily.log` e exibe no terminal ao fechar: horas de convivência, teclas digitadas, carinhos, sacudidas, builds de sucesso e intervenções de fadiga.

---

## 📊 Monitor de Telemetria e Spikes em Terminal ANSI

Utilitário executável compilado em **C11 puro** (`tools/caddin_monitor.c`):
- Conecta-se à FIFO `/tmp/caddin_telemetry.fifo` e plota em tempo real com cores ANSI 256:
  - Níveis gráficos de Dopamina, Noradrenalina, Serotonina e Cortisol.
  - Estágio de vida e porcentagem exata do freio ontogenético da janela crítica.
  - Matriz visual dos 120 pools biológicos da SNN e atividade dos 2048 neurônios a 10 Hz.
  - Nível de fadiga e ações ativas do Bandit UCB1.

---

## 🎛️ Motor de Regras Customizáveis (.caddinrc)

Crie um arquivo `.caddinrc` na pasta do executável ou em `$HOME/.caddinrc`:

```ini
[privacy]
keepass
bitwarden
1password
private window
bank

# Se estiver compilando código no terminal, fica alerta e curioso
[rule:build_focus]
window = gcc
action = react_curious
param = 0.8
cooldown = 15.0

# Se estiver assistindo vídeos tarde da noite, induz calma
[rule:night_youtube]
window = youtube
hour_min = 23
hour_max = 6
action = inject_ser
param = 0.1
cooldown = 60.0

# Em editores de código, estimula dopamina e atenção
[rule:editor_flow]
window = code
action = inject_da
param = 0.08
cooldown = 30.0
```

---

## 🛡️ Persistência Atômica v2 com Integridade CRC32

Todas as memórias do pet são salvas sem risco de corrupção:
- `caddin.brain`: Pesos sinápticos e topologia da rede SNN.
- `caddin.onto`: Estado ontogenético, estágios de vida e imprinting acústico.
- `caddin.sdm`: Memória episódica hiperdimensional distribuída.
- `caddin.bandit`: Modelo de aprendizado por reforço UCB1 de intervenções de fadiga.
- `caddin.keys`: Perfil biométrico de ritmo de digitação do dono.
- `caddin_daily.log`: Histórico diário de convivência e foco.

Cada arquivo utiliza cabeçalho com Magic Number (`CADD`), contagem de versão, timestamp e checksum **CRC32 (IEEE 802.3)**, gravados através de escrita atômica em arquivo temporário seguida de substituição atômica (`rename()` no POSIX / `MoveFileExA` no Windows).

---

## 📁 Estrutura de Diretórios do Projeto

```
caddin/
├── CMakeLists.txt            # Build system para Linux e Windows
├── Makefile                  # Comandos: all, test, windows, run, clean
├── README.md                 # Documentação completa do projeto
├── .caddinrc.example         # Exemplo de regras customizáveis e privacidade
├── behavior.c/.h             # Instintos base
├── conditioning.c/.h         # Condicionamento clássico e operante
├── homeostasis.c/.h          # Balanço homeostático
├── memory.c/.h               # Memória associativa legada
├── network_data.h            # Topologia de rede e estruturas neurais
├── neuro.c/.h                # Módulo neuromórfico auxiliar
├── runtime_types.h           # Tipos fundamentais do runtime
├── sensors.c/.h              # Interface agregadora sensorial
├── cmake/
│   └── mingw-w64.cmake       # Toolchain para cross-compile Windows
├── src/
│   ├── main.c                # Loop principal (60 FPS, dt fixo, zero malloc)
│   ├── config.h              # Constantes, física, janelas e limiares
│   ├── animation/            # animation, autonomy, emotions, softbody
│   ├── behavior/             # bond, dance, dmn, explore, hide_hunt, perch, startle_relief
│   ├── body/                 # audio_out, palette, voice_synth
│   ├── core/                 # arena, bandit, config_rules, cpg, daily_summary, event_ring,
│   │                         # fixed, neuromod, ontogeny, persist, predict, rituals, rng,
│   │                         # sdm, snn, telemetry_fifo
│   ├── lang/                 # lexicon
│   ├── mind/                 # workspace
│   ├── platform/             # window.h, window_win32, window_x11, x11_cursor
│   ├── renderer/             # face, pixel_draw
│   └── sense/                # fatigue, heatmap, keystroke_biometrics, luma, mic, mouse,
│                             # music, scroll, sysload, text, windows
├── tests/                    # 13 suítes completas de testes unitários automatizados
└── tools/
    └── caddin_monitor.c      # Monitor de telemetria em terminal ANSI 100% C
```

---

## 📋 Histórico de Versões e Evolução

| Versão | Neurônios | Arquitetura | Destaque |
|---|---|---|---|
| **v1.4.0** | **2048** (512 + 1152 + 384) | Ontogenia biológica, SDM 2048b, UCB1, Biometria, Telemetria | Estágios de maturação, Kanerva SDM, curiosidade Schmidhuber, Bandit UCB1, fadiga, rituais, biometria IKD, monitor ANSI em C puro |
| **v1.2.0** | 2048 (512 + 1152 + 384) | SNN escalonada, persistência v2 CRC32, multimodal | Hide/hunt, perch, susto→alívio, microfone e leitura de texto |
| **v1.1.6** | 1024 (256 + 768) | SNN LIF com CSR escalável a 2048+ | 11 vozes emocionais, `voice_synth_trigger_ex()`, mapeamento por `state_timer` |
| **v1.1.5** | 1024 | Áudio desacoplado do motor, modulação emocional | Voz ligada às emoções, cooldowns por tipo, fix do reflexo nociceptivo |
| **v1.1.4** | 1024 | Injeção aferente de 6 sensores, DMN/Bond | ALSA/WinMM non-blocking, scroll, janela ativa, 3 comportamentos autônomos |
| **v1.1.3** | 1024 (loop) / 2048 (benchmark CSR) | Q8.8, CSR, R-STDP trifatorial, atraso sináptico | CPG marcapasso, Global Workspace, predição temporal |
| **v1.0.2** | 1024 (denso) | Matriz sináptica clássica com BCM | Release inicial: softbody e backends X11/Win32 |

---

## 🛠️ Guia de Compilação, Cross-Compile e Deploy

### Compilação Nativa no Linux:
```bash
make
make test
```

### Compilação Cruzada para Windows:
```bash
make windows
# Gera executáveis estáticos e portáteis:
# build-windows/caddin.exe
# build-windows/caddin_monitor.exe
```

---

## 🧪 Suíte de Testes Automatizada (13/13 Aprovadas)

O projeto conta com **13 suítes completas de testes automatizados**, cobrindo 100% dos subsistemas neurais, sensoriais e corporais:

```bash
cd build && ctest --output-on-failure
```

```
Test project /home/carlos/Antigravity/caddin/build
      Start  1: CoreTest .........................   Passed    (0.04 sec)
      Start  2: SnnTest ..........................   Passed    (0.34 sec)
      Start  3: BiologyTest ......................   Passed    (0.00 sec)
      Start  4: SensorsTest ......................   Passed    (0.01 sec)
      Start  5: BodyTest .........................   Passed    (0.01 sec)
      Start  6: MindLangTest .....................   Passed    (0.01 sec)
      Start  7: NewModulesTest ...................   Passed    (0.04 sec)
      Start  8: OntogenyTest .....................   Passed    (0.05 sec) [3.877 verificações]
      Start  9: ConfigRulesTest ..................   Passed    (0.01 sec)
      Start 10: SdmTest ..........................   Passed    (0.01 sec) [100% de similaridade]
      Start 11: RitualsTest ......................   Passed    (0.01 sec) [Ciclo circadiano e marcos]
      Start 12: Fase5Test ........................   Passed    (0.01 sec) [EventRing, UCB1, Fadiga]
      Start 13: Fase6Test ........................   Passed    (0.01 sec) [Biometria, Resumo, Telemetria]

100% tests passed, 0 tests failed out of 13
```

---

## 🎮 Controles, Interações e Atalhos

| Entrada | Efeito Físico e Reação Neurobiológica |
|---|---|
| **Arrastar (Botão Esquerdo)** | Segura o corpo mole com estiramento inercial e reação tátil imediata. |
| **Passar o Cursor Gentilmente** | Afeto e carinho: estimula dopamina, serotonina e dispara ronronares confortáveis. |
| **Sacudir o Mouse com Violência** | Reflexo nociceptivo espinhal: contração brusca (squish), choque e squeak agudo de susto. |
| **Roda do Mouse (Scroll)** | Provoca cócegas e pequenas agitações laterais. |
| **Barra de Espaço** | Impulso cinético vertical com salto elástico. |
| **Setas Direcionais (← / ↑ / → / ↓)** | Aplica forças cinéticas nas 4 direções ortogonais. |
| **Tecla 'R'** | Reseta a habituação sensorial da SNN, restaurando a sensibilidade a novidades. |

---

## 📄 Licença

Desenvolvido com carinho e rigor técnico por **HololiveStudios**.  
Código aberto sob a licença **MIT Open Experimental**.

# 🐾 Caddin (v1.0.2)
### Mascote Virtual Autônomo com Cérebro SNN de 1024 Neurônios e Física de Corpo Mole

O **Caddin** é um companheiro virtual para desktop com inteligência biológica emergente. Ao contrário de modelos estáticos de aprendizado de máquina ou scripts com árvores de decisão pré-definidas, o Caddin opera como uma **simulação neurobiológica ativa (Tabula Rasa)** em tempo real: ele aprende e adapta seu comportamento a partir de suas interações diárias através de uma **Rede Neural Pulsada (Spiking Neural Network - SNN)** com plasticidade sináptica local STDP e neuromodulação dopaminérgica.

Além do cérebro biológico, o Caddin possui um **motor 2D ultraleve** com física procedural de corpo mole (*softbody*), janela translúcida flutuante e **consumo de memória ultrabaixo (~7 MB de RAM)**, rodando de forma 100% nativa tanto no **Linux (X11)** quanto no **Windows (Win32)** sem depender de OpenGL, Mesa, LLVM ou bibliotecas pesadas.

---

## 📑 Sumário
1. [Destaques da Versão 1.0.2](#-destaques-da-versão-102)
2. [Arquitetura Detalhada do Código](#-arquitetura-detalhada-do-código)
   - [1. Motor Neurobiológico SNN (`neuro.c`, `neuro.h`)](#1-motor-neurobiológico-snn-neuroc-neuroh)
   - [2. Sistema Sensorial Biológico (`sensors.c`, `sensors.h`)](#2-sistema-sensorial-biológico-sensorsc-sensorsh)
   - [3. Homeostase e Vitais (`homeostasis.c`, `homeostasis.h`)](#3-homeostase-e-vitais-homeostasisc-homeostasish)
   - [4. Comportamento e Ação Motora (`behavior.c`, `behavior.h`)](#4-comportamento-e-ação-motora-behaviorc-behaviorh)
   - [5. Memória e Condicionamento (`memory.c`, `conditioning.c`)](#5-memória-e-condicionamento-memoryc-conditioningc)
   - [6. Física de Corpo Mole Softbody (`src/animation/softbody.c`)](#6-física-de-corpo-mole-softbody-srcanimationsoftbodyc)
   - [7. Renderizador 2D Nativo por CPU (`src/renderer/`)](#7-renderizador-2d-nativo-por-cpu-srcrenderer)
   - [8. Camadas de Plataforma (`src/platform/`)](#8-camadas-de-plataforma-srcplatform)
3. [Instalação e Uso no Linux](#-instalação-e-uso-no-linux)
   - [Instalação Rápida via Script Bash (`install.sh`)](#instalação-rápida-via-script-bash-installsh)
   - [Execução Direta do Binário](#execução-direta-do-binário)
   - [Compilação a partir do Código-Fonte](#compilação-a-partir-do-código-fonte)
4. [Instalação e Uso no Windows](#-instalação-e-uso-no-windows)
5. [Controles e Interações](#-controles-e-interações)
6. [Persistência de Memória Sináptica](#-persistência-de-memória-sináptica)

---

## 🌟 Destaques da Versão 1.0.2

- **Integração Total SNN (Fim dos Atalhos Manuais):** Todos os cliques, movimentos de mouse, agitações e intervalos de teclas são convertidos em taxas de disparo para 64 canais sensoriais. O humor e a reação do Caddin são 100% emergentes do cérebro SNN.
- **Redução Drástica de Memória RAM (-91%):** Substituição completa do stack OpenGL/GLFW por renderizadores nativos de CPU (X11 XShm no Linux e Win32 GDI `UpdateLayeredWindow` no Windows). O uso de RAM caiu de **83 MB para apenas ~7.3 MB**.
- **Compressão da Matriz Sináptica (int8):** Quantização dos pesos basais para `int8_t`, economizando ~2.4 MB no heap sem comprometer a resolução temporal das sinapses.
- **Física de Arremesso e Gravidade Fluida:** Arraste o pet e solte-o no ar para arremessá-lo pela tela com inércia, resistência do ar e amortecimento nas bordas da área útil da área de trabalho.
- **Compilação Cruzada Windows Nativa:** Suporte a geração direta de binário `.exe` 64-bit estático sem dependências externas via MinGW-w64.
- **Instalador Bash Linux Automatizado:** Script `install.sh` que verifica dependências, instala o binário e gera atalho `.desktop` no menu do sistema.

---

## 🧠 Arquitetura Detalhada do Código

O projeto é estruturado em módulos C11 estritamente desacoplados:

```text
caddin/
├── neuro.c / neuro.h          # Rede Neural Pulsada (1024 neurônios LIF, STDP, BCM)
├── sensors.c / sensors.h      # 64 canais sensoriais: física do mouse, ritmo e tempo
├── homeostasis.c / .h         # Vitais biológicos (energia, saciedade, afeto, dopamina)
├── behavior.c / .h            # Mapeamento motor para estados emocionais e ações
├── memory.c / conditioning.c  # Memória episódica e reflexos associativos
├── runtime_types.h            # Definições de tipos comuns e constantes neurais
├── network_data.h             # Parâmetros de topologia e conexões neurais
├── src/
│   ├── main.c                 # Laço principal de alta resolução e sincronização
│   ├── renderer/
│   │   ├── pixel_draw.c/.h    # Rasterizador 2D por software, primitivas e blends
│   │   └── face.c/.h          # Renderização procedural de olhos, pupilas e boca
│   ├── animation/
│   │   ├── softbody.c/.h      # Física elástica (Mass-Spring-Damper) de corpo mole
│   │   ├── animation.c/.h     # Interpolação de poses e respiração procedural
│   │   ├── emotions.c/.h      # Dinâmica de humor e suavização de expressões
│   │   └── autonomy.c/.h      # Deambulação, saltos e comportamentos espontâneos
│   └── platform/
│       ├── window.h           # Interface abstrata de janela e eventos
│       ├── window_x11.c       # Backend Linux (Xlib + XShm shared memory)
│       ├── window_win32.c     # Backend Windows (Win32 GDI + UpdateLayeredWindow)
│       └── x11_cursor.c/.h    # Rastreamento de cursor global na tela
├── bin/                       # Executáveis pré-compilados (Linux & Windows)
├── cmake/mingw-w64.cmake      # Toolchain para compilação cruzada Windows
├── install.sh                 # Instalador automatizado para Linux
└── CMakeLists.txt / Makefile  # Sistemas de compilação
```

---

### 1. Motor Neurobiológico SNN (`neuro.c`, `neuro.h`)

O cérebro do Caddin é composto por **1024 neurônios LIF (*Leaky Integrate-and-Fire*)** organizados biologicamente:

- **256 Neurônios Sensoriais (64 canais × 4 neurônios por canal):** Convertem estímulos físicos externos em potenciais de ação (spikes) com dinâmica de disparo em rajada (*bursting*) e habituação sensorial.
- **576 Interneurônios Recorrentes (32 pools neurais):** Processam emoções secundárias, alerta, memória de trabalho e estados de curiosidade. Obedecem ao princípio de Dale, com subpopulações excitatórias e inibitórias que mantêm o equilíbrio E/I da rede.
- **192 Neurônios Motores (24 pools motores × 8 neurônios por pool):** Ditam a postura corporal, dilatação pupilar, piscar de olhos, expressões de orelhas e impulsos físicos autônomos.

#### Dinâmica de Membrana LIF:
Para cada neurônio $i$:
$$\tau_m \frac{dV_i}{dt} = -(V_i - V_{\text{rest}}) + R_m \cdot I_{\text{syn}}(t)$$
Quando $V_i \ge V_{\text{threshold}}$, o neurônio emite um spike, entra em período refratário absoluto e reseta seu potencial.

#### Plasticidade Sináptica STDP (*Spike-Timing-Dependent Plasticity*):
As sinapses sofrem modificações em tempo real de acordo com a ordem e diferença temporal dos disparos ($\Delta t = t_{\text{pós}} - t_{\text{pré}}$):
- Se $\Delta t > 0$ (o pré disparou antes do pós), a conexão é fortalecida: $\Delta w = A_+ e^{-\Delta t / \tau_+}$.
- Se $\Delta t < 0$ (o pré disparou depois do pós), a conexão é enfraquecida: $\Delta w = -A_- e^{\Delta t / \tau_-}$.

#### Neuromodulação Dopaminérgica:
O aprendizado é trifatorial: a alteração sináptica $\Delta w$ é ponderada pelo nível de recompensa imediato ($R$) fornecido pela homeostase (ex: carinho libera dopamina, quedas ou solavancos excessivos geram estresse e inibição).

#### Estabilidade Homeostática (BCM):
Para impedir saturação dos pesos sinápticos, um processo de homeostase passiva aplica decaimento sináptico contínuo e renormalização vetorial, garantindo que a rede nunca congele em disparo contínuo ou silêncio perpétuo.

---

### 2. Sistema Sensorial Biológico (`sensors.c`, `sensors.h`)

Em vez de simples eventos de clique que alteram variáveis fixas, o módulo `sensors.c` analisa a **física dos gestos** capturados pelo hardware:

1. **Aceleração e Agitação (*Shake Magnitude*):**
   - Agitação lenta/suave (velocidade moderada e contínua) é processada como afeto e toque reconfortante.
   - Agitação rápida/violenta (acelerações bruscas) dispara estímulos de susto, vertigem e perda de equilíbrio.
2. **Pressão e Ritmo de Interação:**
   - Detecta cliques curtos (curiosidade/alerta) vs cliques sustentados (carinho contínuo).
   - O ritmo de digitação ou movimento do mouse modula a percepção de pulso temporal e ciclo circadiano do pet.
3. **Habituação Sináptica:**
   - Estímulos repetitivos contínuos provocam decaimento na resposta dos canais correspondentes (como em organismos reais), fazendo com que o pet pare de reagir a toques insistentes até que ocorra uma pausa.

---

### 3. Homeostase e Vitais (`homeostasis.c`, `homeostasis.h`)

Mantém os drivers vitais que regulam a sobrevivência e os humores básicos:
- **Energia:** Decai com o tempo e com movimentos rápidos; recupera-se quando o pet dorme.
- **Saciedade:** Consumida metabolicamente ao longo das horas.
- **Afeto:** Necessidade biológica de contato social que, quando baixa, induz comportamentos de busca e carência.
- **Fadiga:** Aumenta com o tempo de vigília e interações contínuas, induzindo o sono reparador.
- **Dopamina e Neurotransmissores:** Modulam a plasticidade sináptica e o ganho excitatório dos interneurônios.

---

### 4. Comportamento e Ação Motora (`behavior.c`, `behavior.h`)

Coleta os spikes emitidos pelos **24 pools motores** da SNN e sintetiza os comandos em:
- **Expressões Faciais:** Feliz, Triste, Sonolento, Surpreso, Bravo, Curioso, Brincalhão, Neutro e Piscadela.
- **Vocalizações e Sons:** Emissão de chilreios, pulsos e ronronar simulado.
- **Locomoção Autônoma:** Vagar para esquerda/direita, saltar, dançar, deitar e balançar a cauda/orelhas.

---

### 5. Memória e Condicionamento (`memory.c`, `conditioning.c`)

- **Condicionamento Clássico (Pavloviano):** Associa estímulos neutros (ex: uma tecla específica ou som) a recompensas ou punições após repetições consistentes.
- **Memória Episódica:** Registra os últimos estados de alta ativação emocional para fornecer contexto temporal contínuo ao cérebro SNN.

---

### 6. Física de Corpo Mole Softbody (`src/animation/softbody.c`)

O corpo do Caddin não é um sprite rígido, mas uma malha elástica simulada via **Sistema Massa-Mola-Amortecedor (*Mass-Spring-Damper*)**:
- **Squish & Stretch:** O corpo achata quando pressionado contra o chão ou acelerado verticalmente, e se estica dinamicamente na direção da velocidade durante arremessos.
- **Respiração Procedural:** Pequenas oscilações senoidais sincronizadas com os vitais de fadiga e sono.
- **Inércia Física:** Ao mover a janela bruscamente pelo cursor, o miolo e a face do pet reagem com atraso elástico realista.

---

### 7. Renderizador 2D Nativo por CPU (`src/renderer/`)

- Desenho em canvas RGBA de **128 × 96 pixels** com profundidade de 32 bits.
- **Upscaling Bilinear Preciso:** O canvas é escalado suavemente para **192 × 144 pixels** em tempo real com filtragem bilinear e pré-multiplicação de canal Alpha ($0 \le \alpha \le 255$).
- Elimina completamente o uso de pipelines 3D (OpenGL/Vulkan/DirectX), evitando o custo de instanciar drivers de vídeo e shaders para desenhos 2D simples.

---

### 8. Camadas de Plataforma (`src/platform/`)

#### Linux X11 (`window_x11.c`)
- Utiliza **Xlib + XShm** (extensão de memória compartilhada do MIT).
- Cria uma janela flutuante com suporte a transparência por pixel via visual de 32 bits (ARGB) com gerenciador de composição.
- Uso de memória RAM residente medido em **apenas 7.3 MB**.

#### Windows Win32 (`window_win32.c`)
- Utiliza a API nativa do Windows com **`UpdateLayeredWindow`** e uma DIB Section de 32-bit (`CreateDIBSection`).
- Aceleração direta pelo DWM (*Desktop Window Manager*) com suporte a transparência alfa por pixel real sem bordas ou janelas retangulares feias.
- Medição de tempo ultraprecisa com `QueryPerformanceCounter` e captura global de mouse com `GetCursorPos`.

---

## 🐧 Instalação e Uso no Linux

### Instalação Rápida via Script Bash (`install.sh`)

O Caddin inclui um script instalador em Bash que realiza todas as verificações, compila caso necessário, copia o binário para o sistema e adiciona o atalho no menu de aplicativos da sua interface gráfica (XFCE, GNOME, KDE, etc.):

```bash
# 1. Dê permissão de execução ao script
chmod +x install.sh

# 2. Execute o instalador
./install.sh
```

O instalador irá:
1. Verificar a presença das bibliotecas nativas `libX11` e `libXext`.
2. Instalar o binário em `/usr/local/bin/caddin` (ou `~/.local/bin/caddin`).
3. Criar o atalho `~/.local/share/applications/caddin.desktop`.

Para iniciar o pet após a instalação:
- Digite `caddin` em qualquer terminal, ou
- Busque por **Caddin** no menu de aplicativos do seu sistema.

#### Para Desinstalar:
```bash
./install.sh --uninstall
```

---

### Execução Direta do Binário Pré-compilado

Se preferir rodar sem instalar no sistema, você pode usar o executável otimizado já disponível no repositório:

```bash
chmod +x bin/caddin
./bin/caddin
```

---

### Compilação a partir do Código-Fonte

Caso queira recompilar manualmente no Linux:

#### Pré-requisitos (Debian/Ubuntu):
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libx11-dev libxext-dev
```

#### Compilação simples:
```bash
make
./build/caddin
```

---

## 🪟 Instalação e Uso no Windows

O Caddin roda de forma 100% nativa em **Windows 10 e Windows 11 (64-bit)** sem necessidade de instaladores complexos ou DLLs adicionais:

### Execução Imediata:
1. Baixe ou acesse o arquivo pré-compilado na pasta [`bin/caddin.exe`](file:///home/carlos/Antigravity/caddin/bin/caddin.exe).
2. Dê um duplo clique em `caddin.exe`.
3. O Caddin surgirá no canto inferior direito da sua área de trabalho como uma criatura flutuante e translúcida!

### Compilar para Windows no Linux (MinGW-w64):
Você pode gerar o `.exe` diretamente no Linux com:
```bash
make windows
```
O executável gerado será colocado em `build-windows/caddin.exe`.

---

## 🎮 Controles e Interações

### Interações com o Mouse:
- **Arrastar Suavemente:** Segure o botão esquerdo e mova devagar para acariciar ou passear com o Caddin.
- **Arremesso (Throw):** Arraste com velocidade e solte o botão esquerdo para lançá-lo na tela. O pet sofrerá gravidade, aceleração e rebaterá suavemente nas margens do monitor.
- **Clique Rápido:** Desperta a atenção do Caddin, emitindo estímulos nos canais sensoriais de curiosidade.

### Interações pelo Teclado:
Com a janela do Caddin em foco:
- **`C`**: Fazer carinho (+Afeto, +Dopamina, Reforço STDP).
- **`P`**: Cutucar / Provocar (+Alerta, -Calma).
- **`F`**: Alimentar o Caddin (+Saciedade, +Energia).
- **`T`**: Conversar / Chamar atenção (+Curiosidade).
- **`R`**: Resetar habituação dos neurônios sensoriais.
- **`Esc`**: Salvar o estado do cérebro e fechar com segurança.

---

## 💾 Persistência de Memória Sináptica

O aprendizado do seu Caddin nunca é perdido!
- Durante a execução, a cada **60 segundos** o estado completo da matriz de conexões neurais e pesos sinápticos é salvo automaticamente no arquivo **`caddin.brain`**.
- Ao fechar o aplicativo (pressionando `Esc` ou enviando sinal de término), a persistência final é gravada.
- Quando o Caddin é iniciado novamente, ele reconhece o arquivo `caddin.brain` (ou o legado `cortex_pet.brain`) e restaura a personalidade e memórias acumuladas!
- Caso queira recomeçar a criação do zero como uma **Tabula Rasa**, basta deletar o arquivo `caddin.brain`.

---

## 📄 Licença e Créditos

Desenvolvido com foco em biologia computacional, neurociência de sistemas e engenharia de software de altíssimo desempenho.
- **Linguagem:** C11 puro.
- **Licença:** MIT License.

# Pestinha Neural - Simulador SNN 1024 Neurônios (Terminal C99)

Versão minimalista e ultraleve do **Pestinha Neural**, re-arquitetada para rodar **diretamente no terminal (ASCII/ANSI)** em **C99 puro**, sem nenhuma dependência gráfica externa (como SDL2 ou X11).

---

## 🚀 Destaques da Arquitetura

1. **Rede Neural Pulsada (SNN) com 1024 Neurônios LIF**:
   - **64 Canais Sensoriais** (256 neurônios): Ritmo de digitação, tempo inativo, interações do usuário, ciclo circadiano, pulso temporal e neurotransmissores.
   - **32 Pools Interneurais** (576 neurônios): Processamento emocional, alerta, curiosidade, memória e estados cognitivos.
   - **24 Pools Motores** (192 neurônios): Controle das expressões ASCII no terminal e comportamentos emocionais.

2. **Plasticidade Sináptica STDP (Spike-Timing-Dependent Plasticity) + Homeostase**:
   - Janela temporal de aprendizado baseada na diferença relativa de tempo ($\Delta t = t_{\text{pós}} - t_{\text{pré}}$).
   - Modulação trifatorial por recompensa/dopamina.
   - Relaxamento e homeostase sináptica passiva para evitar saturação de pesos.

3. **Interface ASCII/ANSI Responsiva**:
   - Rosto dinâmico em ASCII (`( ^ _ ^ )`, `( O _ O ) !`, `( - _ - ) zZ`, etc.).
   - Telemetria de vitais e estatísticas da SNN em tempo real.
   - Teclas interativas sem bloqueio (modo `termios` no Linux/macOS).

4. **Desempenho Extremo**:
   - Compilação limpa em C99 sem dependências externas.
   - Executável de ~40 KB e consumo de memória RAM < 2 MB.

---

## 🛠️ Como Compilar e Executar

### 1. Requisitos
- Compilador C padrão (`gcc` ou `clang`).
- Biblioteca matemática padrão (`libm`).

### 2. Compilação
No terminal, dentro da pasta do projeto:

```bash
make
```

### 3. Execução
```bash
./pestinha_term
```
Ou simplesmente:
```bash
make run
```

---

## 🎮 Controles Interativos no Terminal

Enquanto o simulador estiver rodando:

- **`C`**: Fazer carinho no Pestinha (+Afeto, -Estresse, Recompensa STDP)
- **`P`**: Cutucar o Pestinha (+Estresse, +Alerta, Punição STDP)
- **`F`**: Alimentar o Pestinha (+Saciedade, +Energia)
- **`T`**: Conversar com o Pestinha (+Curiosidade)
- **`R`**: Resetar a habituação dos neurônios sensoriais
- **`Q`**: Salvar e Sair do programa
# caddin

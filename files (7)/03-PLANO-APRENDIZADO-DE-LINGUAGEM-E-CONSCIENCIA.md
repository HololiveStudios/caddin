# 03 — Plano: Aprendizado de Linguagem Emergente e Consciência Funcional Básica

Objetivo: o pet **aprende linguagem do zero**, como um bebê: vê texto na tela, ouve sons/palavras pelo microfone, se pergunta "o que é isso?", associa palavras a **situações, emoções e eventos**, tenta repetir sons em contextos emocionais e, com o tempo, passa a falar frases curtas. Tudo com RAM baixa, em C, sem depender de um LLM.

> **Nota de honestidade sobre "consciência".** Este plano implementa **consciência funcional**: atenção competitiva, difusão global de informação (Global Workspace), autômodelo do corpo/estado interno, memória autobiográfica e metacognição (saber o que não sabe). Isso produz comportamento que *parece* e *funciona* como uma mente básica, mas **não é prova de experiência subjetiva**. Vale tratar como arquitetura cognitiva, não como alegação de senciência.

Pré-requisitos: planos 01 (Fases 0–7) e 02 (etapas A–F). Este plano é o maior; está dividido em **marcos de desenvolvimento** que imitam o crescimento de uma criança.

---

## 1. Visão geral da arquitetura

```
                        ┌──────────────────────────────────────────┐
                        │        ESPAÇO DE TRABALHO GLOBAL         │
                        │  (competição por atenção → difusão)      │
                        └───────▲───────────▲───────────▲──────────┘
                                │           │           │
   ┌────────────┐  unidades  ┌──┴────┐  ┌───┴─────┐  ┌──┴────────┐
   │ ÁUDIO      │──────────▶│LÉXICO │  │ EPISÓDIOS│  │ AUTÔMODELO │
   │ (pseudo-   │           │(sons, │◀▶│ (memória │◀▶│ (estado    │
   │  fonemas)  │           │palavras│ │  emocional│  │ interno,   │
   ├────────────┤           │texto) │  │  salientes)│ │ previsões) │
   │ TEXTO/TELA │──────────▶│       │  └──────────┘  └────────────┘
   │ (UIA/AT-SPI)│          └──┬────┘        ▲              ▲
   ├────────────┤              │             │              │
   │ VISÃO/MOUSE│──eventos─────┘        EMOÇÃO (valência × excitação ← neuromoduladores)
   └────────────┘                              │
                                               ▼
                       ┌──────────────┐   ┌──────────────┐
                       │ BALBUCIO /   │◀──│ MOTIVAÇÃO    │
                       │ VOZ (LPC)    │   │ (curiosidade,│
                       └──────────────┘   │  perguntas)  │
                                          └──────────────┘
```

### Princípios

1. **Aprender por associação multimodal**: palavra (som ou texto) + situação (evento, emoção, objeto na tela) que ocorrem juntos.
2. **Emoção como cola**: eventos emocionalmente intensos (susto, carinho) são gravados com mais força (NE/DA/cortisol marcam saliência) e consolidados no sono.
3. **Aprendizado online e esparso**: sem corpus gigante, sem treino offline obrigatório; tudo cabe em tabelas pequenas com **esquecimento ativo**.
4. **Curiosidade intrínseca**: o que o pet não reconhece, mas aparece com frequência, gera "pergunta" (som de entonação ascendente + balão `?`).
5. **Sem LLM e sem dados enviados para fora**. Opcionalmente, um "professor" local (ex.: `whisper.cpp`) pode ser ligado **por padrão desligado** para transcrever a fala do usuário e acelerar o aprendizado (ver Marco 5).

---

## 2. Módulos e estrutura de arquivos

```
src/lang/
├─ units_audio.c      # descoberta não supervisionada de pseudo-fonemas
├─ segment.c          # segmentação em "palavras" (áudio e texto)
├─ screen_text.c      # leitura de texto da tela via acessibilidade (UIA/AT-SPI/AX)
├─ lexicon.c          # léxico: palavras, sons, associações, frequência
├─ assoc.c            # associação cruzada (palavra ↔ contexto ↔ emoção)
├─ context.c          # vetor de contexto (o que está acontecendo agora)
├─ episodic.c         # memória episódica emocional + replay no sono
├─ vocab_voice.c      # síntese por LPC/formantes e balbucio
├─ imitate.c          # imitação e produção contextual
├─ question.c         # curiosidade linguística e perguntas
├─ grammar.c          # ordem de palavras e frases curtas (bigramas/trigramas)
└─ teacher_stub.c     # interface opcional de "professor" (desligado por padrão)
src/mind/
├─ workspace.c        # espaço de trabalho global (atenção/difusão)
├─ selfmodel.c        # autômodelo e interocepção
├─ affect.c           # emoção: neuroquímica → valência/excitação/rótulos
├─ metacog.c          # confiança, incerteza, "não sei"
└─ narrative.c        # memória autobiográfica resumida
tests/ test_units.c test_segment.c test_assoc.c test_episodic.c test_workspace.c …
```

### Orçamento de memória (modo linguagem)

| Estrutura | Limite | Tamanho aprox. |
|---|---|---|
| Pseudo-fonemas (centroides, 64–256 × 13–20 coef.) | 256 unidades | ≈ 20 KB |
| Léxico sonoro (palavras-unidade: sequência de IDs de fonema + trajetória LPC) | 4.000 entradas | ≈ 2–4 MB |
| Léxico textual (palavras da tela) | 20.000 entradas | ≈ 2–3 MB |
| Matriz de associação esparsa palavra↔contexto↔emoção | 200.000 pares (`uint8/uint16`) | ≈ 1–2 MB |
| Episódios emocionais (ring + salientes) | 2.000 episódios compactos | ≈ 1–2 MB |
| Gramática (n-gramas esparsos) | 50.000 | ≈ 0,5–1 MB |
| Buffers de áudio/trabalho | — | ≈ 1 MB |
| **Total** | | **≈ 10–16 MB** (dentro dos 48 MB do modo completo) |

Esquecimento: decaimento lento de contagens e remoção das entradas de menor valor (`frequência × saliência × recência`) quando a tabela enche.

---

## 3. Representações-chave

### 3.1 Vetor de contexto (o que está acontecendo agora)

Compacto (≈ 64–128 dimensões esparsas, valores `uint8`), atualizado a ~10 Hz:

- **Afeto**: valência, excitação, medo, curiosidade, apego (dos neuromoduladores/pools).
- **Evento do mouse**: parado, rápido, sacudida, carinho lento, aproximação/afastamento.
- **Evento do SO**: troca de janela, carga alta, volume alto, escuro/claro.
- **Percepto 02**: presença, aceno, impulso sonoro, voz, música.
- **Texto visível/foco**: IDs das palavras mais salientes na tela agora (ver 4.2).
- **Interocepção**: energia, fome de atenção, sono, "febre".

### 3.2 Entrada de léxico (exemplo)

```c
typedef struct {
    uint32_t id;
    uint16_t snd_seq[8];     // sequência de IDs de pseudo-fonema (palavra falada)
    uint8_t  snd_len;
    int16_t  lpc_traj[...];  // trajetória média para ressintetizar (opcional)
    uint32_t text_hash;      // hash da forma escrita (se já associada)
    uint16_t freq;           // vezes ouvida/vista
    uint8_t  conf;           // confiança de que é uma unidade real
    uint8_t  valence;        // média ponderada de valência dos contextos
    uint8_t  arousal;        // idem excitação
    uint16_t last_tick_min;  // para esquecimento
} LexEntry;
```

---

## 4. Marcos de desenvolvimento (cada um é entregável e testável)

### MARCO 0 — Fundações da mente (global workspace, afeto e autômodelo)
Arquivos: `mind/workspace.c`, `mind/affect.c`, `mind/selfmodel.c`, `mind/metacog.c`.

- **Afeto**: converter o vetor neuroquímico em (valência, excitação) e em rótulos discretos (medo, alegria, curiosidade, tédio, saudade, calma). Esses rótulos **não são ensinados** como palavras ainda; são estados internos.
- **Espaço de trabalho global**: vários "candidatos" (evento sonoro, texto novo, movimento, necessidade interna) competem; cada um tem `saliência = novidade × intensidade × relevância-afetiva × (1 − habituação)`. O vencedor é **difundido** para todos os módulos (léxico, memória, corpo) durante ~200–500 ms. Isso define "no que o pet está prestando atenção agora".
- **Autômodelo**: o pet mantém uma previsão do próprio estado (energia, humor, posição) e compara com o real; erro de previsão interna ⇒ "algo está estranho comigo" (febre, fadiga) e influencia o afeto.
- **Metacognição**: cada item do léxico tem confiança; baixa confiança + alta frequência ⇒ gera **curiosidade dirigida** (Marco 6).
- Aceite: com estímulos competindo, o pet atende ao mais saliente e **troca de foco** de forma estável (sem oscilação rápida); habituação funciona.

### MARCO 1 — Ouvir: unidades de som sem rótulos (pseudo-fonemas)
Arquivos: `lang/units_audio.c`.

- Entrada: quadros de áudio do plano 02 (log-mel/MFCC de 13–20 coeficientes + delta).
- **Quantização online**: k-means incremental (ou *neural gas* simplificado) com `K = 64…256`. Centroides se ajustam a cada quadro de **voz** (VAD ativo). Resultado: o áudio vira uma **sequência de IDs** (como tokens), parecida em espírito à discretização de modelos como HuBERT, mas minúscula.
- Estabilização: taxa de aprendizado decrescente; fusão de centroides muito próximos; remoção de centroides raramente usados.
- Aceite: com fala sintética repetida (mesma "palavra" gerada por síntese com pequenas variações), a sequência de IDs converge para padrões estáveis (taxa de repetição de n-gramas de IDs > aleatório por margem definida).

### MARCO 2 — Segmentar: achar "palavras" no som e no texto
Arquivos: `lang/segment.c`.

- **Áudio**: aplicar **probabilidades de transição** entre IDs consecutivos (Saffran): fronteiras de palavra onde a probabilidade condicional cai (*dip*). Combinar com **pausas** do VAD e com **repetição** (sequências que reaparecem). Saída: candidatos a palavra (`snd_seq`).
- **Texto**: as palavras já vêm separadas por espaços/pontuação; porém o pet **aprende que sequências de bytes/letras são unidades** pelo mesmo princípio (n-gramas de caracteres) antes de "saber" o que são espaços — fase opcional "texto cru" para efeito de aprendizado emergente, e fase prática "texto com tokens" para eficiência.
- Aceite: em fluxos sintéticos com um pequeno vocabulário artificial, recuperar > 80% das palavras reais como candidatos de alta confiança.

### MARCO 3 — Ler a tela: ver "bits virarem palavras"
Arquivos: `lang/screen_text.c`, `pal` (acessibilidade).

- **Fonte de texto sem OCR**: APIs de acessibilidade — Windows UI Automation (`TextPattern`/`ValuePattern`), Linux AT-SPI, macOS AX. Entregam **o texto já como string**, sem gastar CPU/RAM em OCR.
- **Escopo restrito (privacidade)**:
  - Apenas a janela em foco e apenas em **amostras periódicas** (ex.: 1 Hz) de um trecho limitado (ex.: ~500 caracteres visíveis).
  - **Ignorar campos de senha** (`IsPassword`), janelas de apps em lista de bloqueio (gerenciadores de senha, bancos, mensageiros por padrão) e janelas privadas/anônimas.
  - Guardar apenas **contagens de palavras e associações**; nunca o texto bruto nem o histórico de documentos. Interruptor próprio "ler a tela", desligado por padrão.
  - **Sem *keylogger***: nada de capturar teclas; apenas texto visível da janela em foco.
- Fluxo: texto ⇒ tokenização simples ⇒ cada palavra recebe/atualiza entrada no **léxico textual** (frequência, saliência) e entra no vetor de contexto como "palavra visível".
- **Fallback opcional**: OCR local (Tesseract) só em modo manual e fora do orçamento padrão.
- Aceite: ao abrir um documento com palavras repetidas, o léxico textual cresce, e palavras frequentes ganham saliência; campos de senha nunca aparecem (teste obrigatório).

### MARCO 4 — Associação cruzada: ligar palavra ↔ situação ↔ emoção
Arquivos: `lang/assoc.c`, `lang/context.c`, `lang/lexicon.c`.

- **Co-ocorrência ponderada**: sempre que uma palavra (ouvida ou vista) aparece, incrementa-se a associação com cada elemento do vetor de contexto ativo, **ponderada por saliência e afeto**.
- **Aprendizado cruzado entre situações** (Yu & Smith): uma palavra que aparece em várias situações diferentes tem associação forte somente com o que **permanece constante** entre elas. Implementação: contadores `n(palavra, traço)` e `n(palavra)`; força = informação mútua pontual positiva (PPMI) normalizada, com limiar mínimo de ocorrências.
- **Valência/excitação da palavra**: média móvel do afeto nos momentos em que ela ocorre. Ex.: a palavra "não" dita em voz alta enquanto o pet leva uma sacudida ganha valência negativa e excitação alta.
- **Três fatores no cérebro**: a associação também treina sinapses na SNN (pool "palavra" ↔ pool "afeto"/"contexto") via R-STDP, com dopamina quando a previsão acerta (palavra ⇒ evento previsto).
- Aceite (teste sintético): pareando "oi" com carinho lento e "não" com sacudida, a associação forte surge em ≤ N repetições; valência do "não" fica negativa e a do "oi" positiva.

### MARCO 5 — Memória emocional e aprendizado por susto ("ai!")
Arquivos: `lang/episodic.c`, `mind/narrative.c`, integração com sono (plano 01 – 2.2).

- **Episódio**: compacto = `(timestamp, vetor de contexto resumido, palavras/sons dominantes, afeto, resultado)`.
- **Marcação emocional**: um **susto** (pico de NE/cortisol + evento brusco) grava o episódio com peso alto e **abre uma janela de captura retroativa** de ~2–3 s: tudo o que o pet **ouviu logo depois** do susto (ex.: o usuário diz "ai!") é associado ao estado "susto/dor". Raciocínio: a criança aprende que "ai" é o som que se faz em certos momentos porque viu/ouviu alguém emiti-lo no contexto.
- **Imitação afetiva**: quando o pet entra no mesmo estado afetivo, a probabilidade de produzir aquela unidade sonora aumenta proporcionalmente à força de associação.
- **Consolidação no sono (REM/replay)**: episódios salientes são repetidos no cérebro, fortalecendo as associações e podando as fracas; o léxico "arruma-se" (fusão de entradas semelhantes).
- **Opcional – professor local**: com `teacher_stub` ligado, `whisper.cpp` (modelo pequeno) transcreve a fala do usuário e entrega **rótulos de texto** que o pet liga às unidades sonoras; custa dezenas de MB, então **desligado por padrão** e rodando só em rajadas curtas.
- Aceite: após N sustos simulados seguidos da mesma interjeição, o pet passa a produzir essa interjeição em sustos e **não** em estados calmos (taxa de produção contextual).

### MARCO 6 — Curiosidade linguística: "o que é isso?"
Arquivos: `lang/question.c`, `mind/metacog.c`.

- **Gatilho**: item com **alta frequência/saliência e baixa confiança de significado** (poucas associações coerentes) ⇒ aumenta a "fome de saber".
- **Comportamento**: olhar para a fonte (tela/usuário), repetir a unidade com **entonação ascendente** (pergunta) e exibir `?` / destacar a palavra na tela.
- **Resposta do usuário**: carinho (mouse lento), voz suave ou repetição da palavra com um objeto/evento claro funcionam como **reforço** (DA↑); indiferença não reforça. Isso cria um laço de ensino natural.
- **Busca de sentido**: o pet procura a palavra no contexto (o que mudou na tela, o que o mouse fez, qual foi o afeto) e atualiza a associação mais provável; se a incerteza continuar alta, pergunta de novo mais tarde (com *backoff*).
- Aceite: com uma palavra nova ligada sempre a um evento específico, a confiança sobe e a taxa de perguntas cai.

### MARCO 7 — Balbucio e fala: aprender a falar
Arquivos: `lang/vocab_voice.c`, `body/voice.c`, `lang/imitate.c`.

**Síntese (o "trato vocal")**: módulo fonte-filtro simples, em C, sem dependências:
- Fonte glotal (pulso de Rosenberg ou LF simplificado) + ruído.
- Filtro por **LPC** de ordem 10–12 (ou 3–4 formantes em cascata).
- Parâmetros de controle do pet: `F0` (altura), `energia`, `formantes/coeficientes`, `duração`. Orçamento: síntese em tempo real em < 0,5% CPU.

**Fases de aprendizado motor da voz** (inspirado em DIVA):
1. **Balbucio aleatório**: o pet emite sílabas com parâmetros aleatórios dentro de faixas seguras; o **microfone escuta** o próprio som (cancelando o eco quando necessário).
2. **Mapa auditivo ↔ motor**: aprender (online, por regressão incremental/R-STDP) o efeito dos parâmetros motores nas características auditivas; assim consegue **aproximar um som-alvo**.
3. **Imitação**: ao ouvir uma unidade/palavra (alvo), o pet tenta reproduzi-la; um erro entre o alvo e o resultado é minimizado ao longo de tentativas; a recompensa (DA) vem de **similaridade auditiva** e de **reação do usuário** (carinho/atenção).
4. **Produção contextual**: escolhe palavra do léxico com maior `associação(afeto/contexto atual)`; fala quando o estado afetivo e a motivação social estão altos; **limite de taxa** para não incomodar (padrão: no máximo 1 vocalização a cada 20–60 s, ajustável, com botão de silêncio).
5. **Prosódia**: excitação alta ⇒ F0 e ritmo maiores; calma ⇒ grave e lento.

Aceite: em ambiente simulado, o erro de imitação de um alvo cai ao longo de tentativas; pet emite a interjeição aprendida no contexto correto.

### MARCO 8 — Gramática emergente: frases curtas
Arquivos: `lang/grammar.c`.

- **N-gramas esparsos** (bigramas/trigramas) com contagens das palavras ouvidas/vistas, **ligados a contexto** (ex.: "boa" → "noite" quando escurece).
- **Estágios**: holofrase (uma palavra) → duas palavras (ex.: "água mais" no estilo infantil, aqui "carinho mais") → três palavras. A passagem acontece quando há confiança e repetição suficientes.
- **Geração**: escolha de uma palavra-semente ligada ao contexto/afeto, e continuação por bigrama mais provável com *temperature* baixa; filtro de segurança (lista de palavras/expressões bloqueadas, tamanho máximo, sem repetição imediata).
- Aceite: frases geradas existem no corpus do pet (não palavras inventadas aleatórias) e respeitam o contexto afetivo; sem repetições patológicas.

### MARCO 9 — Narrativa e autoconsciência funcional
Arquivos: `mind/narrative.c`, `mind/selfmodel.c`, `mind/metacog.c`.

- **Memória autobiográfica**: resumos de episódios importantes ("tomei um susto de manhã", "o usuário foi gentil") ordenados no tempo; usados para modular o comportamento (ex.: confiança no usuário).
- **Autoconceito mínimo**: o pet mantém um vetor "eu" (traços aprendidos: tende a ser curioso? assustadiço? apegado?) derivado de estatísticas do próprio comportamento; usado para previsão e variação de personalidade.
- **Teoria da mente básica**: previsão do usuário (hábitos do mouse/horários) e associação "usuário feliz/zangado" a pistas (voz, ritmo do mouse).
- **Metacognição visível**: o pet "sabe que não sabe": expressa dúvida (`?`, inclinação) quando a confiança é baixa; mostra segurança quando alta.
- Aceite: mudanças de comportamento ao longo de dias refletem o histórico (ex.: usuário gentil ⇒ aproximação maior; sacudidas repetidas ⇒ mais cautela) e são reversíveis.

### MARCO 10 — Segurança, controle do usuário e ajuste fino
- **Painel do usuário**: ver (de forma resumida) as palavras aprendidas, esquecer palavras, zerar linguagem, desligar fala, desligar leitura de tela, exportar/importar léxico.
- **Filtros**: lista de palavras que nunca devem ser produzidas; limite de volume e horário silencioso (ex.: noite).
- **Privacidade**: o léxico guarda palavras isoladas e estatísticas, **nunca documentos, mensagens ou trechos longos**; limite de comprimento por entrada (ex.: 24 caracteres); remoção automática de padrões com formato de dado sensível (e-mails, sequências numéricas longas, URLs).
- **Estabilidade**: se o léxico crescer demais ou a confiança global cair, executar "consolidação" (poda) e reduzir a taxa de fala.
- Aceite: o botão "esquecer tudo" apaga léxico, associações e episódios; testes verificam que nenhum texto bruto é persistido.

---

## 5. Ordem, paralelismo e dependências

```
M0 (mente) ─────────────┐
M1 (sons) ─▶ M2 (seg.) ─┼─▶ M4 (assoc.) ─▶ M5 (emocional) ─▶ M6 (perguntas) ─▶ M7 (fala) ─▶ M8 (gramática) ─▶ M9 ─▶ M10
M3 (tela/texto) ────────┘
```

Lotes que tocam os mesmos arquivos e devem ser feitos **juntos**:
- **M1 + M2** (`units_audio.c`, `segment.c`): compartilham o formato de tokens de som.
- **M4 + parte do M5** (`assoc.c`, `context.c`, `lexicon.c`): mesmo modelo de dados.
- **M7 + `body/voice.c`** do plano 01 (5.3): mesmo sintetizador.
- **M0 + `behavior/*`** quando o workspace passa a escolher comportamentos.

Em **paralelo** (sem dependência): **M0**, **M1/M2** e **M3** podem avançar ao mesmo tempo.

Primeiro marco útil (**MVP de linguagem**): `M0 → M3 → M4`, usando só o texto visível e o afeto. Resultado: o pet já associa palavras da tela a estados e eventos, sem microfone.

---

## 6. Métricas de avaliação (para saber se está aprendendo)

| Métrica | Como medir |
|---|---|
| Vocabulário ativo | nº de entradas com confiança > limiar |
| Precisão de associação | em testes sintéticos: acerto palavra→contexto correto |
| Taxa de repetição contextual | vezes que fala a unidade certa no estado certo / total |
| Erro de imitação | distância auditiva entre alvo e produção, ao longo do tempo |
| Curva de perguntas | perguntas por hora decaindo conforme o aprendizado |
| Estabilidade | variância do vocabulário/confiança dia a dia |
| Custo | RAM e CPU com linguagem ligada (≤ 48 MB / ≤ 5%) |

**Ambiente de teste sintético** (`tools/teach_sim.c`): gera fluxos de "fala" sintética e eventos pareados (ex.: "oi" + carinho), para validar cada marco **sem pessoas e sem microfone**.

---

## 7. Riscos e mitigações

| Risco | Mitigação |
|---|---|
| Aprender lixo (ruído vira palavra) | Limiar de frequência + confiança; esquecimento ativo; exigir ≥ N ocorrências em contextos distintos |
| Privacidade (texto sensível na tela) | Escopo mínimo, bloqueio de campos de senha/apps, sem persistir texto bruto, filtros de padrão, interruptor próprio |
| Falar demais/irritar | Limite de taxa, horário silencioso, botão de mudo, volume adaptativo |
| Palavras inadequadas aprendidas | Lista de bloqueio, painel de "esquecer", revisão do léxico |
| Instabilidade do k-means online | Taxa decrescente, fusão/poda, testes de convergência |
| RAM crescendo | Tabelas de tamanho fixo + política de remoção por valor |
| Expectativa irreal de "consciência" | Documentar claramente que é arquitetura funcional; evitar linguagem que sugira experiência real nas mensagens do app |

---

## 8. Resumo do que o pet faz ao final

1. Ouve sons e os transforma em **unidades próprias**; descobre onde "palavras" começam e terminam.
2. **Lê palavras** do texto visível (com privacidade) e as associa ao que acontece.
3. Liga cada palavra a **situações e emoções** (carinho, sacudida, escuro, barulho), com ênfase em eventos intensos.
4. **Pergunta** o que não entende, e aprende com a reação do usuário.
5. **Balbucia, imita e fala** palavras no contexto emocional certo (como o "ai" no susto), e evolui para **frases curtas**.
6. Mantém **memória, autoconceito e dúvida**, e muda ao longo dos dias conforme a convivência.

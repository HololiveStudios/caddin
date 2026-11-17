# 00 — Prompt Mestre Melhorado + Pesquisa de Estado da Arte

> Este arquivo contém (1) o que já existe na internet para cada parte do projeto, (2) as decisões de "usar vs. construir", e (3) o prompt mestre reescrito para ser colado em qualquer assistente de código (ou usado como especificação de projeto).
>
> Arquivos relacionados:
> - `01-PLANO-IMPLEMENTACAO.md` — plano principal (núcleo SNN, SO, corpo, cognição, persistência)
> - `02-PLANO-PERCEPCAO-MICROFONE-CAMERA.md` — percepção por microfone e câmera
> - `03-PLANO-APRENDIZADO-DE-LINGUAGEM-E-CONSCIENCIA.md` — linguagem emergente e consciência funcional básica

---

## 1. Resultado da pesquisa

### 1.1 Bibliotecas de SNN existentes (referência, não dependência)

| Projeto | Linguagem | O que oferece | Uso para nós |
|---|---|---|---|
| `neuralos-snn` / `gresil` | Rust, `no_std` | LIF com ponto fixo `i16`, sinapses em CSR, STDP pareado, kernel AVX2 em lote, quantização ternária | **Melhor referência de design**: aritmética inteira, sem alocador no caminho quente, CSR direto/reverso. Copiar a ideia, reescrever em C. Licença AGPL: não copiar código, só a arquitetura. |
| `neuromod` | Rust | LIF, Izhikevich, Hodgkin-Huxley, FitzHugh-Nagumo, Hebbian STDP, R-STDP (STDP modulado por dopamina) | Referência para o **R-STDP** (neuromodulação) e para escolher modelos de neurônio. |
| `omega-snn` (citado na comparação do `neuromod`) | Rust | Arquitetura cognitiva com dopamina + noradrenalina + serotonina + acetilcolina | Confirma que o Bloco 2 das 30 ideias tem precedente. |
| `snn2mcu` (tese Politecnico di Torino) | **C** | LIF e Izhikevich em C para microcontroladores, ponto fixo compatível com CMSIS, conectividade densa/recorrente, gerador de C a partir de NIR | **Referência direta em C** de neurônio em ponto fixo e de layout de memória enxuto. |
| `SpikeForge` | Python → C headers | LIF vetorizado, STDP por traços, exporta pesos como `.h` | Ideia útil: **treinar/prototipar fora e exportar pesos** como blob binário. |
| `Spyker` | C++/CUDA | SNN profunda de alta performance, estruturas esparsas e densas | Mostra que representação **esparsa por índices de spike** economiza memória. Não precisamos de GPU. |
| NIR (Neuromorphic Intermediate Representation) | formato | Descrição portável de redes SNN | Opcional: formato de import/export para testar a rede em outras ferramentas. |
| Brian2, NEST, snnTorch | Python/C++ | Simuladores de pesquisa | Usar **apenas offline** para validar dinâmica (comparar curvas de membrana). |

**Conclusão:** não existe uma biblioteca C pronta, leve e com STDP + neuromodulação + persistência que sirva. A melhor decisão é **escrever o núcleo próprio em C** (é pequeno: ~2–3 mil linhas), seguindo o design inteiro/CSR das bibliotecas Rust acima.

### 1.2 Modelos de lógica/aprendizado para integrar (literatura)

| Necessidade | Modelo/Técnica | Referência-chave |
|---|---|---|
| Aprendizado com recompensa | **STDP de três fatores** com *eligibility traces* (traço de elegibilidade × dopamina) | Izhikevich 2007; Frémaux & Gerstner 2016 |
| Fadiga sináptica | **Short-Term Plasticity** (Tsodyks–Markram) | Tsodyks & Markram 1997 |
| Estabilidade da rede | *Synaptic scaling*, plasticidade intrínseca, balanço E/I | Turrigiano; Zenke |
| Ritmo cardíaco/respiração | CPG de meio-centro ou oscilador Wilson–Cowan | Marder & Bucher |
| Surpresa / curiosidade | Codificação preditiva, **erro de predição como recompensa intrínseca** | Friston; Oudeyer & Kaplan |
| Consciência funcional | **Global Workspace** (competição + difusão) | Baars; Dehaene |
| Emoção | Modelo circumplexo valência–excitação mapeado do vetor neuroquímico | Russell 1980 |
| Segmentar palavras de fala/texto sem rótulos | **Probabilidades de transição** entre sílabas/unidades | Saffran, Aslin, Newport 1996 |
| Ligar palavra ↔ coisa/evento | **Aprendizado cruzado entre situações** (cross-situational) | Yu & Smith 2007 |
| Aprender a falar | **Balbucio motor + feedback auditivo** (modelo DIVA) | Guenther |
| Descobrir "fonemas" sem rótulo | Quantização não supervisionada de áudio (k-means online sobre MFCC/log-mel), ideia por trás de HuBERT/wav2vec-U | Hsu 2021; Baevski 2021 |
| Consolidação de memória | Replay durante o sono | McClelland et al. |

### 1.3 Bibliotecas auxiliares em C (leves, de código aberto) — **verificar versão e licença antes de vendorizar**

| Função | Candidato | Observação |
|---|---|---|
| Áudio de entrada/saída/loopback | `miniaudio` (header único, C) | Captura de microfone, reprodução e *loopback* WASAPI. Zero dependências. |
| FFT | `pffft` ou `kissfft` | Pequenas, rápidas, sem alocação grande. |
| Janela/overlay | API nativa (Win32 `UpdateLayeredWindow`; X11 ARGB + XShape; macOS NSWindow) | Mais leve que qualquer framework gráfico. |
| Renderização | **Rasterizador de software próprio** num bitmap pequeno | O pet é pequeno; dispensa GPU e economiza RAM. |
| Imagens (se precisar) | `stb_image` | Header único. |
| Câmera | Media Foundation (Windows), V4L2 (Linux), AVFoundation (macOS) | Pedir sempre resolução mínima (ex.: 160×120). |
| Detecção de rosto | `pico` (detector em C puro, cascata de testes binários) ou detecção própria por movimento/pele | **Opcional**; confirmar licença (a do `pico` tem restrições para uso comercial). |
| Reconhecimento de voz "professor" | `whisper.cpp` | **Opcional e desligado por padrão** (modelo tiny já ocupa dezenas de MB). Veja o plano 03. |
| Ler texto da tela | **APIs de acessibilidade** (Windows UI Automation, Linux AT-SPI, macOS AX) | Muito mais barato e preciso que OCR e já entrega palavras como texto. OCR (Tesseract) só como último recurso. |
| Testes | `unity` ou `munit` (C) | Ou um *test runner* próprio de 100 linhas. |
| Análise | ASan, UBSan, Valgrind, `clang-tidy`, `cppcheck` | Obrigatórios no CI. |

### 1.4 Decisão: usar × construir

| Componente | Decisão | Motivo |
|---|---|---|
| Núcleo SNN, STDP, neuromoduladores, CPG, STP | **Construir** | Pequeno, e o desempenho depende do layout de memória próprio. |
| Percepção do SO | **Construir** (camada fina sobre APIs nativas) | Sem dependências. |
| Renderização/overlay | **Construir** | Menor RAM possível. |
| Áudio de I/O | **Usar** `miniaudio` | Multiplataforma e testado. |
| FFT | **Usar** `pffft`/`kissfft` | Sem motivo para reescrever. |
| Síntese de voz do pet | **Construir** (FM + síntese por LPC/formantes) | É parte do "corpo" do pet. |
| Reconhecimento de fala "pronto" | **Opcional**, desligado | Contradiz a ideia de aprender sozinho e custa RAM. |

---

## 2. Orçamento de recursos (metas duras)

| Recurso | Modo básico (SNN + SO) | Com áudio/câmera | Com linguagem |
|---|---|---|---|
| RAM residente | ≤ 12 MB | ≤ 24 MB | ≤ 48 MB |
| CPU média (1 núcleo) | ≤ 1% | ≤ 3% | ≤ 5% |
| Latência de reação a evento do mouse | ≤ 16 ms | ≤ 16 ms | ≤ 16 ms |
| Alocações no laço principal | **0** | **0** | **0** |
| Tamanho do executável | ≤ 1 MB | ≤ 2 MB | ≤ 3 MB |
| Disco (cérebro salvo) | ≤ 4 MB | ≤ 8 MB | ≤ 32 MB |

Estratégias: ponto fixo inteiro, SoA (struct of arrays), simulação **orientada a eventos** (só processa neurônios com atividade), passo fixo de 1–2 ms com *tick* adaptativo em repouso (cai para 20–50 Hz quando a tela está quieta e o pet dorme), arena de memória pré-alocada, SIMD com despacho em tempo de execução (AVX2/NEON/escalar), *threads* de percepção em baixa prioridade.

---

## 3. PROMPT MESTRE (versão melhorada)

> Copie tudo dentro do bloco abaixo.

```text
PAPEL
Você é um engenheiro sênior de sistemas em C (C11), especialista em redes neurais de
spikes (SNN), sistemas em tempo real e programação de desktop (Win32/X11/macOS).

OBJETIVO
Implementar, em C puro, um "pet digital" autônomo que vive sobre a área de trabalho do
sistema operacional. Ele possui um cérebro baseado em SNN com plasticidade (STDP de três
fatores), neuromoduladores globais, homeostase, ritmos internos (CPG), percepção do SO
(cursor, scroll, janelas, CPU, áudio do sistema, brilho), expressão corporal procedural
(squash/stretch, pupilas, cor emocional, voz sintética) e comportamento emergente.
Módulos opcionais: percepção por microfone/câmera e aprendizado de linguagem emergente
com consciência funcional básica (global workspace + autômodelo).

RESTRIÇÕES NÃO NEGOCIÁVEIS
1. Desempenho: RAM ≤ 12 MB (básico) e ≤ 48 MB (tudo ligado); CPU média ≤ 1–5% de um núcleo.
2. Zero alocação dinâmica no laço principal. Toda memória vem de arenas pré-alocadas.
3. Aritmética inteira (i16/i32, ponto fixo) no caminho quente da SNN. Float só em
   síntese de áudio, visão e renderização.
4. Layout SoA, simulação orientada a eventos, passo fixo determinístico, RNG
   determinístico (xoshiro/PCG) com semente persistida.
5. Dependências externas mínimas e vendorizadas (miniaudio, pffft/kissfft, stb). Todo o
   restante é código próprio. Nada de runtime pesado, nada de frameworks de GUI.
6. Portabilidade via camada PAL (Platform Abstraction Layer): um header, uma
   implementação por SO. O núcleo (`core/`) não inclui nenhum header do SO.
7. Privacidade por projeto: microfone/câmera/leitura de texto só com consentimento
   explícito, indicador visível quando ativos, 100% local, nunca gravar áudio/vídeo bruto
   em disco, nunca registrar teclas digitadas, ignorar campos de senha.
8. Persistência atômica e versionada (arquivo temporário + rename), com checksum.

PROCESSO
- Trabalhe por fases e lotes conforme `01-PLANO-IMPLEMENTACAO.md`. Cada lote declara os
  arquivos que toca; lotes que tocam os mesmos arquivos são feitos juntos.
- Antes de codar um lote: liste interfaces (headers), invariantes e testes.
- Escreva primeiro testes e *benchmarks* do núcleo; só então otimize.
- Compile com -Wall -Wextra -Werror -std=c11; rode ASan/UBSan; sem warnings.
- Cada lote termina com: código, testes passando, medição de RAM/CPU contra o orçamento.
- Se faltar contexto, adote a suposição mais comum e registre-a em `docs/DECISOES.md`.

ENTREGA
- Código completo e compilável, estrutura de pastas do plano, Makefile/CMake,
  testes unitários, modo headless para testes (sem janela), e relatório de desempenho.
- Respostas apenas com código e caminhos de arquivo quando solicitado em modo de código.

CRITÉRIOS DE ACEITE GLOBAIS
- Rodar 24 h sem crescimento de memória (RSS estável ±2%).
- Mesmo log de entrada + mesma semente ⇒ mesma saída (determinismo).
- Carregar/salvar cérebro corrompido nunca derruba o programa.
- Pet reage a estímulos do mouse em ≤ 16 ms.
```

---

## 4. Como usar

1. Cole o prompt mestre na primeira mensagem de uma sessão de código.
2. Peça: "Execute o Lote X do plano 01" (um lote por vez).
3. Para os módulos opcionais, peça: "Execute a Etapa Y do plano 02/03".
4. Ao final de cada lote, peça o relatório de RAM/CPU e compare com o orçamento da seção 2.

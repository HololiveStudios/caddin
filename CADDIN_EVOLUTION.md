# Carmin — Evolução arquitetural

**Estado:** rede event-driven para 1.048.576 unidades; eventos, Guard, SDM versionada, fila assíncrona limitada e snapshots read-only em progresso; M0–M5 parciais
**Atualizado:** 2026-11-22
**Versão documental:** 0.5

Este é o ponto de entrada para futuras sessões de evolução. Leia este arquivo antes de retomar e consulte apenas os documentos relacionados à etapa atual.

## Identidade e escopo

- **Nome do produto em evolução:** Carmin.
- **Nome atual do repositório e de compatibilidade:** Caddin. O nome do diretório, binários, configuração `.caddinrc` e arquivos persistidos ainda não foram migrados.
- **Objetivo arquitetural:** manter 1.048.576 unidades neuronais indexáveis com estado individual e conectividade esparsa; validar comportamento, custo e desempenho com medições reais.
- **Escopo da atualização 0.4:** adicionar integração por eventos/lazy, estatísticas e benchmark denso de referência, acelerar resolução das regras CSR por pool e registrar a baseline. Suíte geral não executada.

## Estado observado no repositório

- Linguagem/build: C11, CMake e Make; caminhos Linux/X11, Windows, Android e headless estão declarados no build.
- O caminho ativo de comportamento em `src/main.c` usa `brain_create/brain_step` de `neuro.c`; o SNN de `src/core/snn.*` continua não sendo o decisor do app.
- A rede ativa configura **1.048.576 unidades**, preservando os 120 IDs de grupo: 262.144 sensoriais, 589.824 associativas e 196.608 motoras. Cada população-base usa escala 512.
- A matriz densa temporária foi removida da construção e migração. As regras são amostradas em CSR com fan-out máximo de 64 por unidade, mantendo o custo de armazenamento linear. Esta restrição altera a densidade efetiva de projeções em escala alta.
- Foram nomeados dois slots anteriormente genéricos: `CONTEXT_BINDING` e `UNCERTAINTY_MONITOR`. O inventário registra parâmetros, faixa de IDs e regras de projeção.
- A configuração agora valida grupos únicos/não vazios e referências, pesos e densidades das projeções antes de criar o cérebro. Corrigi as referências inválidas `FRUSTRATED` para `FRUSTRATION`.
- A persistência passa a gravar pesos CSR com checksum e inclui migração dos formatos densos de 2.048 unidades; a migração duplica trilhas por réplica alinhada e normaliza pesos pela escala.
- A configuração possui 201 regras de projeção: 120 grupos únicos, nenhuma referência ausente e soma de 1.048.576 unidades.
- O kernel integra os alvos que receberam drive por eventos; células silenciosas avançam em forma fechada quando despertam. Canais sem estímulo não são amostrados e as propostas sensoriais são geradas por amostragem geométrica com thinning pela eficácia.
- O construtor resolve as regras de projeção uma vez por pool, em vez de pesquisar nomes repetidamente para cada unidade; benchmark criou o cérebro em cerca de 6,85 s neste host.
- Benchmark em 2k/10k/100k/1M e baixa/média/alta atividade: no milhão, ciclo neural (estímulo + `brain_step(16 ms)` + STDP) médio de 4,701 ms por eventos e 51,129 ms na referência densa; em alta atividade, 26,372 ms e 66,169 ms. Hashes lógicos completos coincidiram nas séries comparadas. Veja [`CURVA_ESCALA.md`](docs/carmin/CURVA_ESCALA.md) e [`BASELINE.md`](docs/carmin/BASELINE.md).
- Os canais de sistema para CPU e temperatura agora alimentam a SNN; `PAUSE`, backspace, delete, tab, símbolo e pontuação ganharam codificação de evento/classe. Ruído de microfone disponível alimenta `NOISE`. Seis canais continuam sem produtor confiável e estão identificados no inventário.
- Há módulos para arena, neuromodulação, CPG, predição, persistência, ontogenia, SDM, regras de configuração, sensores, corpo/animação, comportamento, linguagem e telemetria.
- A SDM atual declara 128 localizações e hipervetores de 2.048 bits em `src/core/sdm.h`, bem abaixo das 65.536 localizações propostas no blueprint.
- O índice experimental `SdmEpisodeIndex` suporta 256 episódios exatos, replay/reversão tipados e arquivo `caddin.episodes` com schema/CRC. A aplicação carrega e salva, mas ainda não registra exemplos do fluxo real; por isso não há aprendizado episódico cotidiano integrado.
- A persistência da SDM foi migrada para SDM2: schema/geometria, endereços determinísticos, contadores, total de escritas e CRC32. SDM1 é detectado como legado; o app não recupera seus contadores porque o formato antigo não incluía endereços. A API de associação indexada experimental e simulação temporal não substituem ainda o recall distribuído. Microbenchmark (24 pares) marcou 100% exato, 97,85% com 6,25% de ruído; harness virtual de 720 frames marcou 96,53% numa seed. Consulte `SDM_BENCHMARK.md` e `SIMULACAO.md`.
- `CarminEvent` v1 agora integra 64 escalares por frame de forma síncrona; dados brutos de tela/áudio não são carregados. `CarminTelemetrySnapshot` v1 captura as taxas dos 120 pools, contadores e fingerprint de configuração a 1Hz, mas ainda não publica o snapshot.
- Actuator Guard v1 tem modos Shadow/Assisted/Autonomous em biblioteca isolada, catálogo tipado, allowlist, confirmação, expiração, limite de taxa, cancelamento e auditoria de metadados. Não há executor desktop/UI conectados. `CarminAsyncQueue` limita 16 requests e 256 bytes por payload com deadline/cancelamento/conclusão; nenhum worker LLM está integrado.
- O build inclui uma API neural legada em arquivos de raiz (`neuro.c/.h`) e o núcleo `src/core/snn.*`. O caminho de `neuro.c` está ativo; `src/core/snn.*` ainda não está ligado ao decisor do app.
- Não encontrei no CMake atual os componentes de agente/LLM, o barramento cognitivo do blueprint nem o renderizador holográfico especificado nele.
- `git status` já continha diretórios de build Android não rastreados antes da edição documental; eles foram deixados intactos.

## Arquitetura-alvo em avaliação

O desenho preferido mantém o runtime principal em C11 e separa: entrada de eventos e filtros de privacidade; codificadores; populações neurais com fidelidades diferentes; memória associativa/episódica; estado homeostático; política de ações; e adaptadores assíncronos para modelos externos. O núcleo de tempo real não deve esperar por disco, rede ou LLM. A política, fora da rede e do LLM, permanece autoridade para permitir ações.

O contrato final ainda não está congelado. Em particular, formatos numéricos, mapa fixo de 64 grupos, ponto de 1 ms para toda população, tamanhos de arena, densidade de sinapses e os limites de latência do blueprint precisam de protótipo, medição e revisão. Consulte [Arquitetura e decisões](docs/carmin/ARQUITETURA_ALVO.md).

## Documentos de planejamento

- [Arquitetura-alvo e decisões pendentes](docs/carmin/ARQUITETURA_ALVO.md)
- [Plano de implementação M0–M6](docs/carmin/PLANO_IMPLEMENTACAO.md)
- [Fluxo ativo conferido em `src/main.c`](docs/carmin/FLUXO_ATIVO.md)
- [Plano de eventos e autonomia desktop](docs/carmin/PLANO_EVENTOS_E_DESKTOP.md)
- [Baseline e benchmark do kernel neural](docs/carmin/BASELINE.md)
- [Curva 2k–1M em três cargas](docs/carmin/CURVA_ESCALA.md)
- [Contrato de eventos v1](docs/carmin/CONTRATO_EVENTOS.md)
- [Inventário completo dos 120 grupos e status das entradas](docs/carmin/INVENTARIO_GRUPOS.md)
- [Mapa-alvo desktop para os 120 grupos](docs/carmin/MAPA_GRUPOS_DESKTOP.md)
- [Referências técnicas verificadas](docs/carmin/REFERENCIAS.md)

## Componentes e validação

**Implementado:** escala 512 configurada nos grupos existentes, CSR direto com fan-out máximo 64, vetores de estado em heap, atualização neural por eventos com integração lazy, estatísticas de passo, benchmark denso de referência, tabelas de decaimento passivo e resolução de regras CSR por pool, validação das definições de grupo/projeção, introspecção de grupo/sinapse, serialização CSR e migração compatível por projeção disponível.

**Pendente:** replay de eventos reais e hash de mapa nos checkpoints cerebrais, curva em segunda máquina, reduzir a carga alta, validar comportamento e persistência entre quedas, resolver canais sem produtor, revisar topologia limitada por grau e validar grupos desktop. Actuator Guard e fila assíncrona existem apenas como bibliotecas sem UI, executor, worker externo ou modo selecionável pelo app. A biblioteca de snapshot ainda não publica telemetria externa.

**Verificação nesta atualização:** hashes completos dos benchmarks event/denso coincidiram nas séries documentadas; comparação geométrica/Bernoulli rodou em 30 seeds. Testes focados de eventos, Guard, snapshot, fila assíncrona e SDM passam; benchmark SDM e simulação temporal têm protocolo documentado. Builds Linux/Windows serão executados antes do release. `perf` foi bloqueado por `perf_event_paranoid=3`; tempo real geral e portabilidade em hardware distinto não foram comprovados.

## Decisões documentadas

1. Usar Carmin como identidade dos novos planos, sem fingir que o código e os artefatos legados já foram renomeados.
2. Preservar e medir os módulos atuais antes de substituição.
3. Não incorporar como verdades os números e contratos fechados do blueprint fornecido; tratá-los como hipóteses rastreáveis até validação.
4. Nenhuma dependência nova será adicionada até que um protótipo demonstre vantagem que justifique o custo de integração e distribuição.

## Próxima tarefa exata

Rodar builds Linux/Windows e os testes focados após a nova API SDM; então complementar README e release com limites verificados. Depois executar campanhas multi-seed da simulação, separar e versionar a tabela de validade episódica, ligar replay de eventos reais redigidos, completar inventário sensorial de produtores/consumidores, desenhar UI/worker e validar atuação apenas em sandbox. Consultar os planos M0–M6 e eventos/desktop; M0–M5 continuam parciais e M6 aguarda segundo hardware.

## Histórico resumido

- **0.1 — 2026-11-22:** leitura dos documentos e planejamento inicial Carmin.
- **0.3 — 2026-11-22:** rede configurada para 1.048.576 unidades; construtor CSR com grau máximo; vetores no heap; migração adaptada; compilação concluída, desempenho ainda pendente.
- **0.4 — 2026-11-22:** primeiro kernel por eventos, tabela de regras por pool, benchmark denso de referência e baseline medido em um host; fases de validação e autonomia desktop abertas.
- **0.5 — 2026-11-22:** CarminEvent ligado ao loop; SDM2 com CRC32 e endereços persistidos; benchmark SDM/simulação temporal; Actuator Guard e fila assíncrona limitados; snapshot de telemetria read-only com hash de grupos; validação dos contratos focada; build/release ainda pendentes.

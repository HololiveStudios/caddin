# Carmin — fluxo executável atual

Este mapa descreve o caminho que `main()` executa hoje; nomes semelhantes em módulos não significam que sejam usados pelo decisor ativo.

## Inicialização

1. `src/main.c:main()` cria `Brain` com seed 42 via `brain_create()` de [`neuro.c`](../../neuro.c).
2. Carrega pesos de `caddin.brain`; se falhar, tenta `cortex_pet.brain`. O formato ativo persiste pesos para a topologia CSR configurada.
3. Inicializa `Sensors` (`sensors_open`) e módulos independentes de mouse, scroll, janela/texto, carga/temperatura, luma, heatmap, microfone, música, fadiga, biometria de teclado, eventos, memória e comportamento. Recursos dependentes de plataforma podem estar indisponíveis.
4. Carrega, quando existem, os arquivos de ontogenia/imprint, SDM, bandit, biometria e outros estados persistidos.

## Por frame

1. Atualiza relógio, janela/overlay, posição/cursor, clique/arrasto, colisões e entradas de teclado; alimenta `Sensors` e módulos de fadiga, mouse, scroll, foco/janela, heatmap, texto, som e atividade.
2. `sensors_poll(sensors, sim_t, &sr, inputs)` codifica os canais genéricos. `src/main.c` ainda injeta explicitamente CPU/temperatura em `CH_CPU_LOAD`, `CH_TEMP_WARM/COLD` e loudness em `CH_NOISE`. Ganho sensorial da neuromodulação escala os 64 valores e limita o teto a 1.
3. O vetor passa por um lote síncrono `CarminEvent` v1 (64 valores escalares com sequência, timestamp, canal, proveniência, disponibilidade e flags de privacidade); eventos indisponíveis/bloqueados viram zero. `brain_stimulate(brain, event_inputs)` define a intensidade por canal. `brain_step(brain, dt*1000)` executa subpassos LIF e atualiza as taxas dos 120 pools.
4. A atividade alimenta `cpg_step`, neuromodulação e workspace; a cada 60 ticks as taxas alimentam ontogenia. Recompensa homeostática/condicionada modula `brain_learn_stdp`, chamado **em todo frame**.
5. `behavior_update` e `behavior_update_emotion` interpretam estado, drives e taxas; a saída atual afeta comportamentos e corpo/voz do organismo virtual. Não despacha ações gerais de mouse/teclado/arquivo/shell.
6. Telemetria é enviada pelo canal FIFO quando o organismo está dormindo. Autosave periódico e ao fechar grava pesos, ontogenia, SDM, bandit e estado biométrico.

## Limites de fluxo e fronteiras

- O decisor ativo é `neuro.c/.h`; `src/core/snn.c/.h` é um módulo separado e não substitui essa chamada em `main()`.
- `brain_step` não inclui `brain_learn_stdp`, embora a aplicação chame aprendizado logo depois. O baseline agora mede os dois em separado e juntos.
- Sensores de tela/texto respeitam filtros existentes para algumas categorias sensíveis, mas ainda não há um Actuator Guard comum nem contrato de proveniência/retention aplicado a todos os dados.
- `src/core/event_ring.*` observa eventos de comportamento; não é uma fila universal que roteia eventos de todos os sensores para o cérebro.
- `src/core/cognitive_event.*` é o contrato/adapter síncrono de escalares do loop; não transporta texto/áudio/pixels e não criou uma fila concorrente.
- `src/core/actuator_guard.*` implementa políticas determinísticas Shadow/Assisted/Autonomous em biblioteca isolada; nenhum executor de desktop ou UI de confirmação chama essa política no fluxo atual.
- O índice `SdmEpisodeIndex` (`caddin.episodes`) é carregado/salvo pelo app com schema/CRC e até 256 associações exatas, mas o fluxo atual não chama `sdm_episode_index_store`; não aprende episódios reais ainda.
- A SDM ativa tem 128 localizações com k=8; SDM2 persiste endereços e contadores com CRC32. SDM1 legado omitia endereços, portanto é detectado e reinicializado deterministicamente pelo carregador do app em vez de interpretar contadores com geometria errada. A API tipada de associação ainda é experimental e não armazena validade por chave.
- Persistência é feita pelo loop e pelas funções específicas dos módulos. A chamada de arquivo ocorre fora do kernel SNN, mas o autosave não é uma política de snapshot concorrente.

## Referências de código

- Inicialização e recuperação de arquivos: [`src/main.c`](../../src/main.c), em torno das linhas 395–570.
- Sensores, codificação e chamada neural: `src/main.c`, em torno das linhas 624–970.
- STDP e retorno ao comportamento: `src/main.c`, em torno das linhas 1090–1170.
- Autosave e shutdown: `src/main.c`, em torno das linhas 1330–1385.
- Benchmark isolado e de referência densa: [`bench/bench_neuro_step.c`](../../bench/bench_neuro_step.c) e [`BASELINE.md`](BASELINE.md).

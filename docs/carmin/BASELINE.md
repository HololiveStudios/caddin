# Baseline neural Carmin — 1.048.576 neurônios

**Coleta:** 2026-11-22 · **Máquina:** Intel Core i7-7700T (4c/8t, 2,90 GHz), Linux x86_64, kernel 6.12.113+deb13-amd64 · **Compilador:** GCC 14.2.0 · **Build:** CMake Release.

## Método

See the full build/run commands for all tiers in [`CURVA_ESCALA.md`](CURVA_ESCALA.md).

`bench_neuro_step_1m` executa o kernel por eventos. `bench_neuro_step_1m_dense` usa o mesmo código, seed, topologia, estímulos e amostragem sensorial, mas integra densamente todos os 786.432 estados associativos/motores em cada subpasso. Portanto, a comparação isola a integração neural; ela não compara versões inteiras de commits diferentes. O harness mede separadamente `brain_stimulate`, `brain_step(16 ms)` e `brain_learn_stdp` (chamado todo frame em `src/main.c`). O total não inclui construção de `Brain`, sensores do SO, renderização, comportamento, persistência ou áudio.

Cada processo cria a rede com seed 42, 52.166.656 sinapses e 16 subpassos por frame; após cada passo executa STDP com recompensa fixa 0,1 e `dt=0,016 s`. Além do hash observável de taxas/eficácias, o harness materializa estados lazy, quantiza doubles a `1e-9` e hasheia potenciais, adaptação, refratariedade, spikes recentes, pesos, RNG e taxas. Em todas as três cargas da curva, os hashes completos evento/denso coincidiram. Isso fornece evidência forte para as seeds/séries executadas, mas não substitui teste estatístico da nova amostragem sensorial geométrica contra muitas seeds nem replay de sensores reais.

## Resultados observados

| Carga | Eventos (ciclo completo) | Denso (ciclo completo) | Ganho | Hash lógico completo |
|---|---:|---:|---:|---|
| Baixa: TYPING=0,2 · 30 frames | 0,732 ms | 48,146 ms | 65,8× | `39d8c21350d8ce1c` |
| Média: TYPING=0,8; CPU_LOAD=0,3; TALK=0,1 · 30 frames | 4,701 ms | 51,129 ms | 10,9× | `f40ba519c6877599` |
| Alta: canais 0–7 em 1,0 · 5 frames | 26,372 ms | 66,169 ms | 2,5× | `644b01fda5dd3dfc` |

O ciclo médio fica abaixo de 16 ms, mas o alto leva 26,4 ms. Para a curva completa de 2k, 10k, 100k e 1M neurônios em três cargas, veja [`CURVA_ESCALA.md`](CURVA_ESCALA.md).

## Verificação da amostragem sensorial

A proposta geométrica foi comparada à Bernoulli independente por neurônio usando a mesma dinâmica e topologia de 100k, em 30 seeds (1–30), 30 frames médios por seed. A média foi 3.984,53 spikes sensoriais na geométrica (DP 45,29) contra 3.969,63 na referência Bernoulli (DP 39,08). Diferença pareada média: +14,90 spikes, IC95% [-9,07; 38,87], ou +0,38%; o intervalo inclui zero. Isso apoia a equivalência de distribuição neste cenário e duração; não prova todos os canais, níveis, tempos ou extremos de atividade.

O RSS do processo de 1M variou entre **520.056 e 539.788 KiB** em execuções event-driven e cerca de **534.784 KiB** na referência densa, incluindo runtime, estado, vetores auxiliares e CSR. A criação da rede levou aproximadamente 6,6–8,5 s nos processos medidos; os valores por ciclo excluem essa inicialização. Os contadores de hardware do `perf` não estavam acessíveis neste ambiente (`perf_event_paranoid=3`).

## Limites e próximos critérios

- A carga alta ainda leva 25,3 ms por ciclo de 16 ms simulados (ou 20,2 ms no passo sem STDP); tempo real não está garantido para qualquer workload.
- Comparação foi feita num único host; a curva ainda não foi validada em duas arquiteturas/hardwares.
- Os hashes completos coincidiram nas séries e seeds fixas medidas; executar muitas seeds e validar a distribuição da amostragem geométrica contra Bernoulli por unidade.
- Não há dados de FPS integrado da aplicação, latência de sensores/atuadores ou qualidade comportamental.
- O benchmark permite repetir os cenários com os alvos de escala `2k`, `10k`, `100k`, `1m` e os cenários `idle`, `low`, `medium`, `high`.

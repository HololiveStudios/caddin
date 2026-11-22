# Curva de escala do núcleo Carmin

**Coleta:** 2026-11-22 · Intel Core i7-7700T, Linux x86_64, GCC 14.2, CMake Release.

Cada par executa o mesmo cenário e seed em dois binários: kernel por eventos e referência que integra densamente as unidades associativas/motoras. Cada frame faz `brain_stimulate`, `brain_step(16 ms)` e `brain_learn_stdp(0.1, 0.016)`. O hash lógico materializa o estado lazy e quantiza doubles a `1e-9`; inclui estados LIF, refratariedade, spikes recentes, pesos, sensoriais, RNG e taxas por grupo. Hash igual indica mesmo estado observado dentro da quantização usada, com o limite normal de colisão do hash.

## Resultados por escala e carga

| Unidades configuradas | Carga | Frames | Ciclo por eventos | Ciclo denso | Ganho | Sinapses criadas | Hash lógico 1e-9 |
|---:|---|---:|---:|---:|---:|---:|---|
| 2.048 | Baixa | 2.000 | 0,003 ms | 0,281 ms | 93,7× | 86.176 | `f591307db99a6946` |
| 2.048 | Média | 2.000 | 0,005 ms | 0,078 ms | 15,6× | 86.176 | `61de9908d4afe5da` |
| 2.048 | Alta | 2.000 | 0,008 ms | 0,081 ms | 10,1× | 86.176 | `c29cea2482d1f0a3` |
| 10.240 | Baixa | 30 | 0,008 ms | 0,594 ms | 74,3× | 497.120 | `a84e80dfc9036ef3` |
| 10.240 | Média | 30 | 0,023 ms | 0,407 ms | 17,7× | 497.120 | `b213896f2f448d1f` |
| 10.240 | Alta | 30 | 0,087 ms | 0,464 ms | 5,3× | 497.120 | `fd1b34b956d8dce8` |
| 102.400 | Baixa | 30 | 0,059 ms | 3,955 ms | 67,0× | 5.094.400 | `b831d9f6de37575d` |
| 102.400 | Média | 30 | 0,257 ms | 4,088 ms | 15,9× | 5.094.400 | `4844710a7f5b23ce` |
| 102.400 | Alta | 30 | 0,884 ms | 4,608 ms | 5,2× | 5.094.400 | `9806f5eea734dffb` |
| 1.048.576 | Baixa | 30 | 0,732 ms | 48,146 ms | 65,8× | 52.166.656 | `39d8c21350d8ce1c` |
| 1.048.576 | Média | 30 | 4,701 ms | 51,129 ms | 10,9× | 52.166.656 | `f40ba519c6877599` |
| 1.048.576 | Alta | 5 | 26,372 ms | 66,169 ms | 2,5× | 52.166.656 | `644b01fda5dd3dfc` |

As definições dos cenários e os comandos estão em [`BASELINE.md`](BASELINE.md). A carga alta usa oito canais sensoriais a 1,0; no milhão, o ciclo por eventos leva 26,4 ms, acima do período de 16 ms. O melhor ganho não significa que a aplicação completa opera em tempo real: renderização, leitura de sensores do SO, áudio, persistência e comportamentos não estão cronometrados aqui.

## Bytes da conectividade e RSS

Para cada rede, a CSR corrente ocupa `9 * sinapses + 4 * (neurônios + 1)` bytes: destino `uint32`, peso `float`, base `int8` e offsets `uint32`.

| Unidades | Bytes das colunas + offsets | RSS máximo observado de processo |
|---:|---:|---:|
| 2.048 | 0,75 MiB | ~3,8 MiB (eventos, rodada anterior ao hash lógico completo) |
| 10.240 | 4,31 MiB | ~8,4 MiB (eventos/denso, rodada anterior ao hash lógico completo) |
| 102.400 | 44,1 MiB | ~53–55 MiB (eventos/denso, rodada anterior ao hash lógico completo) |
| 1.048.576 | 451,8 MiB | 520–540 MiB (variação entre processos; hash completo incluído nas medições posteriores) |

O restante é estado neuronal, vetores auxiliares, runtime e alocador. O fan-out médio observado é ~49,75; máximo é 64. A geração procedural das conexões fixas não foi implementada: destinos e pesos continuam armazenados em CSR.

## Comandos reproduzíveis

```sh
cmake -S . -B /tmp/carmin-event-build -DCMAKE_BUILD_TYPE=Release -DCADDIN_HEADLESS=ON
cmake --build /tmp/carmin-event-build --target \
  bench_neuro_step_2k bench_neuro_step_2k_dense \
  bench_neuro_step_10k bench_neuro_step_10k_dense \
  bench_neuro_step_100k bench_neuro_step_100k_dense \
  bench_neuro_step_1m bench_neuro_step_1m_dense -j2
/tmp/carmin-event-build/bench_neuro_step_1m 30 medium
/tmp/carmin-event-build/bench_neuro_step_1m_dense 30 medium
```

Use `idle`, `low`, `medium` ou `high` como segundo argumento. Os alvos de benchmark estão fora do build normal do aplicativo.

## Pendências

- Repetir em hardware/arquitetura adicional; os resultados acima são somente deste host.
- Validar comportamento sob padrões temporais reais e atividade sustentada, não só os perfis sintéticos do harness.
- O evento sensorial usa propostas geométricas por pool e thinning por eficácia; validar sua distribuição contra amostragem independente por neurônio em muitas seeds.
- Otimizar a carga alta e medir o loop completo da aplicação, inclusive renderização e sensores reais.

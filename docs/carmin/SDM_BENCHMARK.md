# Memória episódica: avaliação controlada da SDM

## Protocolo

O alvo `bench_sdm_association` usa seed fixa 42 para a SDM e 24 pares endereço→resultado de 2.048 bits. A consulta executa 2.048 amostras distribuídas igualmente entre os 24 padrões. Por padrão há quatro ciclos de replay dos mesmos pares; o argumento posicional altera a quantidade de ciclos (1–100). O RNG de endereços/resultados e a sequência de ruído também são fixos. Para reproduzir em build headless Release:

```sh
cmake -S . -B /tmp/carmin-build -DCMAKE_BUILD_TYPE=Release -DCADDIN_HEADLESS=ON
cmake --build /tmp/carmin-build --target bench_sdm_association -j2
/tmp/carmin-build/bench_sdm_association 4
```

São registrados acerto por similaridade acima do limiar, falsos negativos, latência total e por consulta, contadores ativos, resultado vazio, interferência por endereços próximos, consolidação, replay e reversão. O baseline é o palpite fixo do primeiro resultado, sem armazenamento. Limiar exato=0,70; ruído=0,60. A etapa de reversão associa cada chave a vetor zero; o reconhecimento de valor neutro tem limitação semântica porque `sdm_recall` retorna apenas `bool` para “há informação”, não distingue um zero aprendido de memória vazia.

## Resultado inicial observado

Host: Intel Core i7-7700T, Linux x86_64, GCC 14.2, CMake Release. Quatro replays idênticos.

| Métrica | Acertos/2048 | Taxa | Observação |
|---|---:|---:|---|
| Palpite fixo sem memória | 86 | 4,20% | Os resultados são aleatórios; baseline não condicionado |
| SDM, chave exata | 2048 | 100% | Similaridade ≥0,70 |
| SDM, 6,25% de bits ruidosos na chave | 2004 | 97,85% | Similaridade ≥0,60 |
| Após consolidação do sono | 2048 | 100% | Sobrevivência nesta sequência curta |
| Após reversão para vetor zero | 0 | 0% | Similaridade ≥0,60, dado antigo não passa limiar |

Latência de recall observada ficou em 24–25 μs por consulta nesta máquina. A gravação dos 24 pares equivale a 192 atualizações de localização × 2.048 contadores por ciclo. Em uma execução inicial, a consolidação levou ~0,05 ms. Esses números são microbenchmark da SDM isolada: não incluem o loop Carmin, carga concorrente, contexto real, retenção de episódios nem significância entre múltiplas seeds.

## Limites encontrados e decisões pendentes

- `sdm_consolidate_sleep` hoje reduz contadores de magnitude maior que 1; ele não remove de forma sistemática o suporte não-zero e não demonstra esquecimento gradual. `sdm_active_counter_count` mede ocupação, não capacidade semântica.
- A memória só tem 128 localizações, k=8 e vetores densos de 2048 bits. Interferência entre 24 episódios, consultas ambíguas e saturação devem ser avaliadas variando cardinalidade e seeds antes de aumentar SDM.
- A API de associação tipada `SdmAssociation` agora distingue memória vazia de uma associação zero e inclui indicador de validade/força agregada. O formato SDM2 ainda persiste a matriz de contadores, não a validade por chave; a semântica booleana antiga continua disponível para compatibilidade.
- Replay do harness regrava pares sintéticos determinísticos. Ainda não é replay de eventos reais capturados no app e não deve incluir texto ou conteúdo privado.
- A gravação SDM2 agora versiona parâmetros, preserva endereços determinísticos, total de gravações e valida CRC32; SDM1 é reconhecido como legado, mas seus endereços ausentes tornam as associações irreconstruíveis. O carregador reinicializa para estado vazio determinístico ao encontrar o formato antigo, evitando associar contadores a endereços incorretos.
- Ainda não há snapshot A/B com rollback para os diversos arquivos da aplicação nem validação da preservação do checkpoint anterior diante de interrupção real de energia.

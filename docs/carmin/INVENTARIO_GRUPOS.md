# Carmin — Inventário e configuração dos grupos

**Fonte dos parâmetros e conexões:** [`network_data.h`](../../network_data.h). **Montagem/introspecção:** [`neuro.c`](../../neuro.c), [`neuro.h`](../../neuro.h). **Configuração-alvo:** 1.048.576 unidades, 120 grupos e 201 regras de projeção, com máximo de 64 sinapses de saída por unidade.

A escala 512 multiplica cada população-base sem deslocar IDs de grupo: 4.096 unidades por canal sensorial, 18.432 por grupo associativo e 8.192 por grupo motor. IDs de grupos permanecem 0–119 e valores de taxa e constantes continuam os perfis declarados no código. `Regras E/S` conta projeções declaradas, não sinapses instanciadas. A topologia amostrada limita o fan-out a 64 para manter o armazenamento proporcional ao número de unidades.

## Distribuição

| IDs | Classe | Grupos | Unidades/grupo | Total |
|---|---|---:|---:|---:|
| 0–63 | Sensorial | 64 | 4.096 | 262.144 |
| 64–95 | Associativo | 32 | 18.432 | 589.824 |
| 96–119 | Motor | 24 | 8.192 | 196.608 |
| **Total** |  | **120** |  | **1.048.576** |

## Sensoriais

`Taxa` em Hz; `d` é depressão por spike; `τ_rec` em ms.

| ID | Grupo | N | Taxa | d | τ_rec | Regras E/S |
|---:|---|---:|---:|---:|---:|---:|
| 0 | `TYPING` | 4096 | 30.0 | 0.15 | 20.0 | 0/2 |
| 1 | `KEY_FAST` | 4096 | 40.0 | 0.20 | 15.0 | 0/2 |
| 2 | `KEY_SLOW` | 4096 | 15.0 | 0.10 | 30.0 | 0/2 |
| 3 | `IDLE` | 4096 | 10.0 | 0.05 | 50.0 | 0/2 |
| 4 | `PAUSE` | 4096 | 12.0 | 0.05 | 40.0 | 0/1 |
| 5 | `CMD_ENTER` | 4096 | 50.0 | 0.25 | 10.0 | 0/2 |
| 6 | `BACKSPACE` | 4096 | 35.0 | 0.15 | 20.0 | 0/1 |
| 7 | `DELETE` | 4096 | 35.0 | 0.15 | 20.0 | 0/1 |
| 8 | `ARROW` | 4096 | 25.0 | 0.10 | 25.0 | 0/1 |
| 9 | `TAB` | 4096 | 20.0 | 0.10 | 25.0 | 0/1 |
| 10 | `SPACE` | 4096 | 30.0 | 0.12 | 20.0 | 0/2 |
| 11 | `SHIFT` | 4096 | 20.0 | 0.08 | 30.0 | 0/0 |
| 12 | `ALPHANUM` | 4096 | 25.0 | 0.10 | 20.0 | 0/1 |
| 13 | `NUMERIC` | 4096 | 25.0 | 0.10 | 20.0 | 0/0 |
| 14 | `SYMBOL` | 4096 | 30.0 | 0.12 | 20.0 | 0/1 |
| 15 | `PUNCTUATION` | 4096 | 20.0 | 0.10 | 25.0 | 0/1 |
| 16 | `CARESS` | 4096 | 60.0 | 0.30 | 10.0 | 0/5 |
| 17 | `POKE` | 4096 | 70.0 | 0.40 | 10.0 | 0/4 |
| 18 | `FEED` | 4096 | 50.0 | 0.25 | 15.0 | 0/3 |
| 19 | `TALK` | 4096 | 45.0 | 0.20 | 15.0 | 0/3 |
| 20 | `GREET` | 4096 | 40.0 | 0.18 | 20.0 | 0/2 |
| 21 | `IGNORE` | 4096 | 15.0 | 0.05 | 40.0 | 0/3 |
| 22 | `REWARD` | 4096 | 65.0 | 0.30 | 10.0 | 0/3 |
| 23 | `PUNISH` | 4096 | 65.0 | 0.35 | 10.0 | 0/3 |
| 24 | `HELP_REQ` | 4096 | 30.0 | 0.15 | 25.0 | 0/0 |
| 25 | `CLEAN` | 4096 | 35.0 | 0.15 | 20.0 | 0/0 |
| 26 | `PLAY` | 4096 | 50.0 | 0.22 | 15.0 | 0/3 |
| 27 | `SLEEP_REQ` | 4096 | 25.0 | 0.10 | 30.0 | 0/0 |
| 28 | `CALL` | 4096 | 45.0 | 0.20 | 15.0 | 0/0 |
| 29 | `WHISTLE` | 4096 | 40.0 | 0.18 | 20.0 | 0/0 |
| 30 | `MUSIC` | 4096 | 35.0 | 0.12 | 25.0 | 0/3 |
| 31 | `NOISE` | 4096 | 55.0 | 0.25 | 15.0 | 0/3 |
| 32 | `TIME_DAY` | 4096 | 10.0 | 0.02 | 100.0 | 0/2 |
| 33 | `TIME_NIGHT` | 4096 | 10.0 | 0.02 | 100.0 | 0/2 |
| 34 | `UPTIME` | 4096 | 15.0 | 0.05 | 50.0 | 0/1 |
| 35 | `CPU_LOAD` | 4096 | 20.0 | 0.08 | 30.0 | 0/2 |
| 36 | `IDLE_LONG` | 4096 | 8.0 | 0.02 | 120.0 | 0/3 |
| 37 | `FOCUS_ACTIVE` | 4096 | 25.0 | 0.10 | 25.0 | 0/3 |
| 38 | `SESSION_LEN` | 4096 | 12.0 | 0.04 | 80.0 | 0/0 |
| 39 | `TICK_PULSE` | 4096 | 20.0 | 0.08 | 30.0 | 0/0 |
| 40 | `TEMP_WARM` | 4096 | 15.0 | 0.05 | 50.0 | 0/2 |
| 41 | `TEMP_COLD` | 4096 | 15.0 | 0.05 | 50.0 | 0/1 |
| 42 | `USER_PRESENT` | 4096 | 30.0 | 0.10 | 20.0 | 0/2 |
| 43 | `USER_AWAY` | 4096 | 10.0 | 0.03 | 80.0 | 0/2 |
| 44 | `QUICK_CLICK` | 4096 | 35.0 | 0.15 | 20.0 | 0/2 |
| 45 | `SLOW_CLICK` | 4096 | 15.0 | 0.05 | 40.0 | 0/1 |
| 46 | `BURST_INPUT` | 4096 | 50.0 | 0.22 | 15.0 | 0/2 |
| 47 | `RHYTHM_REGULAR` | 4096 | 25.0 | 0.10 | 25.0 | 0/2 |
| 48 | `CIRCADIAN_PEAK` | 4096 | 12.0 | 0.04 | 80.0 | 0/2 |
| 49 | `CIRCADIAN_LOW` | 4096 | 12.0 | 0.04 | 80.0 | 0/2 |
| 50 | `FATIGUE_PULSE` | 4096 | 18.0 | 0.06 | 40.0 | 0/2 |
| 51 | `HUNGER_PULSE` | 4096 | 18.0 | 0.06 | 40.0 | 0/2 |
| 52 | `HEARTBEAT_FAST` | 4096 | 30.0 | 0.12 | 20.0 | 0/2 |
| 53 | `HEARTBEAT_SLOW` | 4096 | 15.0 | 0.05 | 50.0 | 0/2 |
| 54 | `BREATHING` | 4096 | 20.0 | 0.08 | 30.0 | 0/1 |
| 55 | `ADRENALINE` | 4096 | 45.0 | 0.20 | 15.0 | 0/3 |
| 56 | `DOPAMINE` | 4096 | 50.0 | 0.22 | 15.0 | 0/3 |
| 57 | `SEROTONIN` | 4096 | 25.0 | 0.10 | 25.0 | 0/3 |
| 58 | `CORTISOL` | 4096 | 40.0 | 0.18 | 18.0 | 0/3 |
| 59 | `OXYTOCIN` | 4096 | 35.0 | 0.15 | 20.0 | 0/3 |
| 60 | `ENDORPHIN` | 4096 | 40.0 | 0.18 | 18.0 | 0/3 |
| 61 | `MEMORY_TRACE` | 4096 | 20.0 | 0.08 | 30.0 | 0/2 |
| 62 | `STDP_SPIKE` | 4096 | 30.0 | 0.12 | 20.0 | 0/1 |
| 63 | `REWARD_WAIT` | 4096 | 22.0 | 0.09 | 25.0 | 0/3 |

## Associativos

`tipo` é a classificação atual (`exc`/`inh`); `τ`, `b` e `τ_b` parametrizam o modelo de potencial/adaptação. O sinal da transmissão é definido por peso explícito.

| ID | Grupo | N | Tipo | τ ms | b | τ_b ms | Regras E/S |
|---:|---|---:|---|---:|---:|---:|---:|
| 64 | `ALERT` | 18432 | exc | 20.0 | 0.05 | 100.0 | 13/3 |
| 65 | `CURIOUS` | 18432 | exc | 22.0 | 0.04 | 120.0 | 9/4 |
| 66 | `CALM` | 18432 | exc | 25.0 | 0.03 | 150.0 | 11/3 |
| 67 | `PLAYFUL` | 18432 | exc | 18.0 | 0.06 | 90.0 | 9/4 |
| 68 | `FRUSTRATION` | 18432 | exc | 16.0 | 0.07 | 80.0 | 4/3 |
| 69 | `GLOOM` | 18432 | exc | 30.0 | 0.02 | 200.0 | 1/1 |
| 70 | `SLEEPY` | 18432 | exc | 35.0 | 0.02 | 220.0 | 6/2 |
| 71 | `HUNGRY` | 18432 | exc | 24.0 | 0.04 | 130.0 | 1/3 |
| 72 | `SATISFIED` | 18432 | exc | 26.0 | 0.03 | 140.0 | 5/3 |
| 73 | `BORED` | 18432 | exc | 28.0 | 0.03 | 160.0 | 4/3 |
| 74 | `SURPRISED` | 18432 | exc | 14.0 | 0.08 | 70.0 | 3/2 |
| 75 | `FEARFUL` | 18432 | exc | 15.0 | 0.08 | 75.0 | 4/2 |
| 76 | `AFFECTIONATE` | 18432 | exc | 22.0 | 0.04 | 110.0 | 3/4 |
| 77 | `LONELY` | 18432 | exc | 32.0 | 0.02 | 190.0 | 3/3 |
| 78 | `HYPERACTIVE` | 18432 | exc | 12.0 | 0.10 | 60.0 | 10/4 |
| 79 | `STUBBORN` | 18432 | exc | 20.0 | 0.05 | 120.0 | 0/1 |
| 80 | `CREATIVE` | 18432 | exc | 21.0 | 0.05 | 115.0 | 0/3 |
| 81 | `MEMORY_HOLD` | 18432 | exc | 40.0 | 0.01 | 300.0 | 1/1 |
| 82 | `REWARD_EXPECT` | 18432 | exc | 19.0 | 0.06 | 100.0 | 3/2 |
| 83 | `THREAT_DETECT` | 18432 | inh | 15.0 | 0.08 | 70.0 | 3/3 |
| 84 | `RHYTHM_GEN` | 18432 | exc | 18.0 | 0.05 | 90.0 | 2/2 |
| 85 | `FOCUS_POOL` | 18432 | exc | 23.0 | 0.04 | 130.0 | 7/2 |
| 86 | `COGNITIVE_LOW` | 18432 | inh | 30.0 | 0.02 | 180.0 | 2/2 |
| 87 | `COGNITIVE_HIGH` | 18432 | exc | 17.0 | 0.07 | 85.0 | 3/2 |
| 88 | `SOCIAL_SEEK` | 18432 | exc | 25.0 | 0.04 | 140.0 | 5/4 |
| 89 | `PAIN_REACTION` | 18432 | inh | 14.0 | 0.09 | 65.0 | 1/3 |
| 90 | `COMFORT_POOL` | 18432 | exc | 27.0 | 0.03 | 150.0 | 5/2 |
| 91 | `ENERGY_CONSERVE` | 18432 | exc | 35.0 | 0.02 | 210.0 | 4/3 |
| 92 | `HABITUATION_POOL` | 18432 | inh | 28.0 | 0.03 | 160.0 | 0/2 |
| 93 | `CONTEXT_BINDING` | 18432 | exc | 28.0 | 0.03 | 160.0 | 3/1 |
| 94 | `UNCERTAINTY_MONITOR` | 18432 | exc | 16.0 | 0.07 | 80.0 | 2/3 |
| 95 | `EXECUTIVE_CTRL` | 18432 | exc | 18.0 | 0.06 | 90.0 | 0/2 |

## Motores

| ID | Grupo | N | τ ms | b | τ_b ms | Regras E/S |
|---:|---|---:|---:|---:|---:|---:|
| 96 | `FACE_HAPPY` | 8192 | 20.0 | 0.05 | 100.0 | 6/0 |
| 97 | `FACE_SAD` | 8192 | 25.0 | 0.04 | 120.0 | 5/0 |
| 98 | `FACE_SLEEP` | 8192 | 30.0 | 0.02 | 180.0 | 3/0 |
| 99 | `FACE_SURPRISED` | 8192 | 15.0 | 0.08 | 70.0 | 3/0 |
| 100 | `FACE_ANGRY` | 8192 | 16.0 | 0.07 | 80.0 | 3/0 |
| 101 | `FACE_CURIOUS` | 8192 | 22.0 | 0.05 | 110.0 | 5/0 |
| 102 | `FACE_PLAYFUL` | 8192 | 18.0 | 0.06 | 90.0 | 2/0 |
| 103 | `FACE_NEUTRAL` | 8192 | 25.0 | 0.03 | 140.0 | 2/0 |
| 104 | `FACE_WINK` | 8192 | 17.0 | 0.07 | 85.0 | 0/0 |
| 105 | `CHIRP_SOUND` | 8192 | 14.0 | 0.08 | 65.0 | 2/0 |
| 106 | `BEEP_PULSE` | 8192 | 15.0 | 0.08 | 70.0 | 1/0 |
| 107 | `PURR_EFFECT` | 8192 | 24.0 | 0.04 | 130.0 | 4/0 |
| 108 | `WANDER_LEFT` | 8192 | 20.0 | 0.05 | 100.0 | 5/0 |
| 109 | `WANDER_RIGHT` | 8192 | 20.0 | 0.05 | 100.0 | 4/0 |
| 110 | `JUMP_ACTION` | 8192 | 14.0 | 0.09 | 60.0 | 1/0 |
| 111 | `DANCE_ACTION` | 8192 | 16.0 | 0.07 | 80.0 | 4/0 |
| 112 | `SLEEP_ACTION` | 8192 | 35.0 | 0.02 | 200.0 | 2/0 |
| 113 | `EAT_ACTION` | 8192 | 22.0 | 0.05 | 110.0 | 1/0 |
| 114 | `CUDDLE_ACTION` | 8192 | 23.0 | 0.04 | 120.0 | 1/0 |
| 115 | `ALERT_EMIT` | 8192 | 15.0 | 0.08 | 75.0 | 3/0 |
| 116 | `NOD_HEAD` | 8192 | 18.0 | 0.06 | 90.0 | 3/0 |
| 117 | `SHAKE_HEAD` | 8192 | 18.0 | 0.06 | 90.0 | 3/0 |
| 118 | `THINK_EMIT` | 8192 | 21.0 | 0.05 | 105.0 | 5/0 |
| 119 | `TAIL_WAG` | 8192 | 19.0 | 0.06 | 95.0 | 6/0 |

## Novos grupos funcionais

- **ID 93 — `CONTEXT_BINDING`:** substitui o placeholder `PLASTIC_NODE1`; recebe `MEMORY_TRACE`, `FOCUS_ACTIVE` e `TAB`; tau=28 ms, b=0,03, tau_b=160 ms; saída principal `THINK_EMIT`.
- **ID 94 — `UNCERTAINTY_MONITOR`:** substitui `PLASTIC_NODE2`; recebe `STDP_SPIKE` e `REWARD_WAIT`; tau=16 ms, b=0,07, tau_b=80 ms; direciona atenção a `FACE_CURIOUS` e reduz `PLAYFUL`/`HYPERACTIVE` por pesos negativos.

Os IDs foram mantidos nos mesmos slots para não deslocar a tabela ontogenética. Os parâmetros e vínculos completos estão nas tabelas acima.

## Canais de entrada

O codificador ativo e `main.c` agora alimentam **58 de 64 canais**. Há fonte explícita para pausa curta, backspace/delete/tab, classes de símbolo/pontuação, carga/temperatura do sistema e ruído alto quando o microfone está disponível. Canais ainda sem produtor confiável no runtime: `SHIFT`, `HELP_REQ`, `CLEAN`, `CALL`, `WHISTLE`, `MUSIC`. `MUSIC` possui detector dedicado mas a atualização ainda não está conectada ao ciclo ativo; `SHIFT`, `HELP_REQ`, `CLEAN`, `CALL` e `WHISTLE` seguem reservados até existir evento seguro e observável.

## Validação de configuração

`brain_create` rejeita nomes duplicados ou ausentes, grupos vazios, peso/taxa não finitos, pesos zerados e densidades fora de (0,1]. A criação exige cobertura exata de 1.048.576 IDs. `brain_pool_info` fornece nome, início, tamanho e classe; `brain_synapse_count` expõe a quantidade CSR criada. As 201 regras têm referências válidas; o número de sinapses é determinado pelo amostrador limitado por projeção.

A construção usa metadados `exc`/`inh`, mas o runtime não aplica sinal de Dale automaticamente; sinais inibitórios são pesos negativos explícitos. Rever isso antes de congelar os perfis.

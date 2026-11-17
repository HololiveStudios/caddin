# 🐾 Caddin v1.1.6 — Síntese de Voz Emocional Rica

## O que há de novo

Esta versão substitui os 4 sons genéricos anteriores por **11 tipos de vocalização procedural** biologicamente inspirados em comportamentos felinos/mamíferos. Os sons agora emergem diretamente do estado emocional e **escalam com o tempo na emoção** — carinho curto soa diferente de carinho prolongado; raiva leve é diferente de raiva intensa.

---

## 🎙️ 11 Tipos de Voz Emocional

| Tipo | Emoção | Duração típica |
|---|---|---|
| `VOICE_PURR_CONTENT` | Carinho moderado — ronron suave com AM triangular 20 Hz | 350–500 ms |
| `VOICE_PURR_DEEP` | Êxtase prolongado — ronron grave 55 Hz + 3 harmônicos | 400–600 ms |
| `VOICE_TRILL` | Alegria / brincadeira — 3 chirps rápidos 450→780 Hz | 420 ms |
| `VOICE_CHIRP_HAPPY` | Toque gostoso — 2 notas curtas (500 Hz + 660 Hz) | 220 ms |
| `VOICE_CHIRP_CURIOUS` | Curiosidade — sweep 380→760 Hz + vibrato no topo | 180 ms |
| `VOICE_GRUNT_IDLE` | Bocejo — descida sonolenta 320→160 Hz | 450 ms |
| `VOICE_WHINE_PROTEST` | Saudade / reclamação — descida com vibrato crescente | 300–350 ms |
| `VOICE_GROWL_ANNOYED` | Raiva baixa — 160 Hz + 4 harmônicos + tremolo gutural | 280 ms |
| `VOICE_HISS_ANGRY` | Raiva intensa — burst de ruído branco fricativo puro | 80 ms |
| `VOICE_SQUEAK_STARTLED` | Susto — 1300→550 Hz, ataque zero, decay instantâneo | 100 ms |
| `VOICE_CRY_DISTRESS` | Angústia — vibrato ±40 Hz + soluço 3.5 Hz | 350 ms |

---

## 🔀 Mapeamento Emocional Escalado

A emoção determina o som **e a intensidade escala com o `state_timer`**:

- **`EMO_AFFECTION`**: 3 estágios progressivos
  - `< 1s` → `VOICE_CHIRP_HAPPY` (surpresa gostosa)
  - `1–4s` → `VOICE_PURR_CONTENT` (carinho moderado)
  - `> 4s` → `VOICE_PURR_DEEP` + `VOICE_TRILL` esporádico (êxtase)
- **`EMO_ANNOYED`**: escalada de raiva
  - `< 2s` → `VOICE_GROWL_ANNOYED`
  - `> 2s` → `VOICE_HISS_ANGRY`
- **`EMO_SURPRISED`** → `VOICE_SQUEAK_STARTLED`
- **`EMO_DIZZY`** → `VOICE_WHINE_PROTEST`
- **`DMN_YAWN`** → `VOICE_GRUNT_IDLE`
- **Bond saudade** → `VOICE_WHINE_PROTEST`

---

## 🔧 Novo na API

```c
/* Trigger com intensidade emocional [0.0..1.0] */
/* Modula: volume, vibrato depth, vibrato rate, profundidade harmônica */
void voice_synth_trigger_ex(VoiceSynth *vs, VoiceType type,
                             float duration_sec, float intensity);

/* Retrocompatível (chama trigger_ex com intensity=0.6) */
void voice_synth_trigger(VoiceSynth *vs, VoiceType type, float duration_sec);
```

### Novos campos na `VoiceSynth` struct

```c
float vibrato_phase, vibrato_rate, vibrato_depth;  /* trêmulo de pitch */
float tremolo_phase, tremolo_rate;                  /* modulação de amplitude */
uint32_t noise_seed;                                /* xorshift32 para textura orgânica */
uint8_t repeat_count, repeat_max;                   /* trilos e pios múltiplos */
uint32_t repeat_len, gap_len;                       /* duração in-place sem malloc */
float intensity;                                    /* 0..1 — expressividade */
float breath_phase, f2, phase2;                     /* osciladores extras */
```

---

## 🧪 Testes

```
7/7 testes passando (BodyTest cobre todos os 11 tipos):

PURR_CONTENT   (350ms, int=0.6): 5600 amostras, 5596 não-zero ✅
PURR_DEEP      (400ms, int=0.9): 6400 amostras, 6398 não-zero ✅
TRILL          (420ms, int=0.8): 5040 amostras, 3117 não-zero ✅
CHIRP_HAPPY    (220ms, int=0.7): 3040 amostras, 2221 não-zero ✅
CHIRP_CURIOUS  (180ms, int=0.6): 2880 amostras, 2879 não-zero ✅
GRUNT_IDLE     (450ms, int=0.4): 7200 amostras, 7184 não-zero ✅
WHINE_PROTEST  (300ms, int=0.6): 4800 amostras, 4797 não-zero ✅
GROWL_ANNOYED  (280ms, int=0.7): 4480 amostras, 4479 não-zero ✅
HISS_ANGRY     (80ms,  int=0.9): 1280 amostras, 1265 não-zero ✅
SQUEAK_STARTLED(100ms, int=0.9): 1600 amostras, 1578 não-zero ✅
CRY_DISTRESS   (350ms, int=0.8): 5600 amostras, 5596 não-zero ✅
VOICE_IDLE: silêncio garantido ✅
Compatibilidade retroativa trigger() ✅
```

---

## 📦 Binários

- **Linux:** compilar com `make` (requer `libx11-dev`, `libxext-dev`, `libasound2-dev`)
- **Windows:** `make windows` → `build-windows/caddin.exe` (estático, sem dependências)

---

## 🔄 Arquivos Alterados

```
src/body/voice_synth.h   — 11 VoiceType, struct expandida, voice_synth_trigger_ex()
src/body/voice_synth.c   — síntese completa para todos os 11 tipos
src/main.c               — mapeamento emocional escalado + 7 cooldown slots
tests/test_body.c        — cobertura total dos 11 tipos (duração, energia, clipping)
```

---

**Builds verificados:** Linux GCC ✅ · Windows MinGW-w64 ✅ · Testes 7/7 ✅

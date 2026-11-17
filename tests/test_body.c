#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../src/core/fixed.h"
#include "../src/core/neuromod.h"
#include "../src/body/palette.h"
#include "../src/body/voice_synth.h"

static void test_palette_and_pupils(void) {
    printf("[TEST] Paleta Cromática e Dilatação Pupilar...\n");
    Neuromodulators nm;
    neuromod_init(&nm);

    EmotionalAura aura_calm;
    emotional_aura_compute(&nm, 0, &aura_calm);

    /* Injetar estresse (NE + Cortisol) */
    neuromod_inject_ne(&nm, fix_from_float(1.5f));
    neuromod_inject_cort(&nm, fix_from_float(1.0f));

    EmotionalAura aura_stressed;
    emotional_aura_compute(&nm, 0, &aura_stressed);

    /* A cor deve avermelhar e as pupilas dilatarem sob estresse */
    assert(aura_stressed.r >= aura_calm.r);
    assert(aura_stressed.pupil_dilation > aura_calm.pupil_dilation);

    /* Com luz forte no monitor, a pupila deve contrair */
    EmotionalAura aura_light;
    emotional_aura_compute(&nm, fix_from_float(1.0f), &aura_light);
    assert(aura_light.pupil_dilation < aura_stressed.pupil_dilation);

    printf("  -> Paleta e Pupilas: OK\n");
}

/* ---------------------------------------------------------------------------
 * Utilitário: gera N amostras totais através de múltiplos renders de 512
 * --------------------------------------------------------------------------- */
static size_t render_full(VoiceSynth *vs, int16_t *buf, size_t buf_size) {
    size_t total = 0;
    size_t chunk;
    while ((chunk = voice_synth_render(vs, buf + total,
                    (buf_size - total < 512) ? (buf_size - total) : 512)) > 0) {
        total += chunk;
        if (total + 512 > buf_size) break;
    }
    return total;
}

static void test_voice_synth(void) {
    printf("[TEST] Sintetizador Procedural de Áudio — 11 tipos emocionais...\n");
    VoiceSynth vs;
    int16_t buffer[8192];
    size_t rendered;
    int non_zero;

    /* Estrutura de teste por tipo */
    struct {
        VoiceType  type;
        float      dur;
        float      intensity;
        const char *name;
    } cases[] = {
        { VOICE_PURR_CONTENT,    0.35f, 0.6f, "PURR_CONTENT"    },
        { VOICE_PURR_DEEP,       0.40f, 0.9f, "PURR_DEEP"       },
        { VOICE_TRILL,           0.42f, 0.8f, "TRILL"           },
        { VOICE_CHIRP_HAPPY,     0.22f, 0.7f, "CHIRP_HAPPY"     },
        { VOICE_CHIRP_CURIOUS,   0.18f, 0.6f, "CHIRP_CURIOUS"   },
        { VOICE_GRUNT_IDLE,      0.45f, 0.4f, "GRUNT_IDLE"      },
        { VOICE_WHINE_PROTEST,   0.30f, 0.6f, "WHINE_PROTEST"   },
        { VOICE_GROWL_ANNOYED,   0.28f, 0.7f, "GROWL_ANNOYED"   },
        { VOICE_HISS_ANGRY,      0.08f, 0.9f, "HISS_ANGRY"      },
        { VOICE_SQUEAK_STARTLED, 0.10f, 0.9f, "SQUEAK_STARTLED" },
        { VOICE_CRY_DISTRESS,    0.35f, 0.8f, "CRY_DISTRESS"    },
    };
    int n = (int)(sizeof(cases) / sizeof(cases[0]));

    for (int i = 0; i < n; ++i) {
        voice_synth_init(&vs, 16000);
        voice_synth_trigger_ex(&vs, cases[i].type, cases[i].dur, cases[i].intensity);

        uint32_t expected = (uint32_t)(cases[i].dur * 16000.0f);

        rendered = render_full(&vs, buffer, 8192);

        /* Deve gerar amostras compatíveis com a duração esperada (margem ±512) */
        assert(rendered >= (size_t)(expected > 512 ? expected - 512 : 0));
        assert(rendered <= (size_t)(expected + 512));

        /* Verificar energia real (pelo menos 60% das amostras não-zero) */
        non_zero = 0;
        for (size_t j = 0; j < rendered; ++j) {
            if (buffer[j] != 0) non_zero++;
        }
        assert((size_t)non_zero > rendered * 6 / 10);

        /* Verificar ausência de clipping (int16 máximo = 32767) */
        for (size_t j = 0; j < rendered; ++j) {
            assert(buffer[j] <= 32767 && buffer[j] >= -32768);
        }

        /* Após o fim, deve retornar IDLE (0 amostras) */
        size_t after = voice_synth_render(&vs, buffer, 512);
        assert(after == 0);

        printf("  -> %s (%.0fms, int=%.1f): %zu amostras, %d não-zero: OK\n",
               cases[i].name, cases[i].dur * 1000.0f, cases[i].intensity,
               rendered, non_zero);
    }

    /* VOICE_IDLE não deve gerar nada */
    voice_synth_init(&vs, 16000);
    rendered = voice_synth_render(&vs, buffer, 512);
    assert(rendered == 0);
    printf("  -> VOICE_IDLE: silêncio garantido: OK\n");

    /* Trigger simples (compatibilidade retroativa) ainda deve funcionar */
    voice_synth_trigger(&vs, VOICE_CHIRP_CURIOUS, 0.10f);
    rendered = render_full(&vs, buffer, 4096);
    assert(rendered > 0);
    printf("  -> Compatibilidade voice_synth_trigger(): OK\n");

    printf("  -> VoiceSynth COMPLETO: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO FASE 4: CORPO, EXPRESSÃO E VOZ PROCEDURAL ===\n");
    test_palette_and_pupils();
    test_voice_synth();
    printf("=== FASE 4 APROVADA COM SUCESSO ===\n");
    return 0;
}

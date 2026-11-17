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

static void test_voice_synth(void) {
    printf("[TEST] Sintetizador Procedural de Áudio (VoiceSynth)...\n");
    VoiceSynth vs;
    voice_synth_init(&vs, 16000);

    /* Testar chirp de pergunta */
    voice_synth_trigger(&vs, VOICE_CHIRP, 0.1f); /* 100 ms = 1600 amostras */

    int16_t buffer[2048];
    size_t rendered = voice_synth_render(&vs, buffer, 2048);
    assert(rendered == 1600);

    /* Verificar que foram geradas amostras com energia real */
    int non_zero = 0;
    for (size_t i = 0; i < rendered; ++i) {
        if (buffer[i] != 0) non_zero++;
    }
    assert(non_zero > 1000);

    /* Após o término, o sintetizador deve voltar a IDLE */
    size_t next_render = voice_synth_render(&vs, buffer, 100);
    assert(next_render == 0);

    printf("  -> VoiceSynth: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO FASE 4: CORPO, EXPRESSÃO E VOZ PROCEDURAL ===\n");
    test_palette_and_pupils();
    test_voice_synth();
    printf("=== FASE 4 APROVADA COM SUCESSO ===\n");
    return 0;
}

#include "palette.h"

void emotional_aura_compute(const Neuromodulators *nm, fix16_t local_light, EmotionalAura *out_aura) {
    if (!out_aura) return;

    /* Base neutra do Caddin: cor suave de gatinho/slime */
    int base_r = 180;
    int base_g = 200;
    int base_b = 220;

    if (nm) {
        /* Noradrenalina / Cortisol altos -> avermelhado/laranja de alerta */
        int stress_component = (nm->cort + nm->ne) >> 2; /* em Q8.8 */
        int r_boost = fix_to_int(stress_component * 40);
        base_r += r_boost;
        base_b -= r_boost / 2;

        /* Dopamina / Serotonina altas -> tom acolhedor verde/dourado */
        int calm_component = (nm->ser + nm->da) >> 2;
        int g_boost = fix_to_int(calm_component * 30);
        base_g += g_boost;
    }

    /* Clamp RGB [0..255] */
    if (base_r > 255) base_r = 255; else if (base_r < 0) base_r = 0;
    if (base_g > 255) base_g = 255; else if (base_g < 0) base_g = 0;
    if (base_b > 255) base_b = 255; else if (base_b < 0) base_b = 0;

    out_aura->r = (uint8_t)base_r;
    out_aura->g = (uint8_t)base_g;
    out_aura->b = (uint8_t)base_b;

    /* Pupilas: luz da tela contrai, alerta (NE) ou afeto dilata */
    int dilation = 128;
    if (nm) {
        dilation += fix_to_int(nm->ne * 50);
    }
    /* Contração por luz local */
    dilation -= fix_to_int(local_light * 40);
    if (dilation > 255) dilation = 255;
    if (dilation < 30) dilation = 30;

    out_aura->pupil_dilation = (uint8_t)dilation;
}

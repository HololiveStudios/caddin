#include "sensors.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Índices dos 64 canais sensoriais (de network_data.h) ── */
enum {
    CH_TYPING = 0, CH_KEY_FAST, CH_KEY_SLOW, CH_IDLE, CH_PAUSE,
    CH_CMD_ENTER, CH_BACKSPACE, CH_DELETE, CH_ARROW, CH_TAB,
    CH_SPACE, CH_SHIFT, CH_ALPHANUM, CH_NUMERIC, CH_SYMBOL, CH_PUNCTUATION,

    CH_CARESS, CH_POKE, CH_FEED, CH_TALK, CH_GREET, CH_IGNORE,
    CH_REWARD, CH_PUNISH, CH_HELP_REQ, CH_CLEAN, CH_PLAY,
    CH_SLEEP_REQ, CH_CALL, CH_WHISTLE, CH_MUSIC, CH_NOISE,

    CH_TIME_DAY, CH_TIME_NIGHT, CH_UPTIME, CH_CPU_LOAD,
    CH_IDLE_LONG, CH_FOCUS_ACTIVE, CH_SESSION_LEN, CH_TICK_PULSE,
    CH_TEMP_WARM, CH_TEMP_COLD, CH_USER_PRESENT, CH_USER_AWAY,
    CH_QUICK_CLICK, CH_SLOW_CLICK, CH_BURST_INPUT, CH_RHYTHM_REGULAR,

    CH_CIRCADIAN_PEAK, CH_CIRCADIAN_LOW, CH_FATIGUE_PULSE,
    CH_HUNGER_PULSE, CH_HEARTBEAT_FAST, CH_HEARTBEAT_SLOW,
    CH_BREATHING, CH_ADRENALINE, CH_DOPAMINE, CH_SEROTONIN,
    CH_CORTISOL, CH_OXYTOCIN, CH_ENDORPHIN, CH_MEMORY_TRACE,
    CH_STDP_SPIKE, CH_REWARD_WAIT
};

struct Sensors {
    /* ── Teclas e digitação ── */
    double last_input_t;
    double key_press_interval;
    int    burst_counter;
    double session_start_t;
    char   last_key;

    /* ── Física da janela ── */
    bool   is_dragging;
    double drag_speed;
    double drag_vx;
    double drag_vy;
    bool   was_clicked;
    bool   was_bumped;

    /* ── Detector de agitação (shake) ── */
    double last_drag_vx;
    int    direction_changes;
    double shake_window_t;
    double shake_intensity;     /* 0..1 acumulador de sacudida */

    /* ── Arraste suave contínuo (carinho) ── */
    double gentle_drag_t;

    /* ── Proximidade do cursor ── */
    int    cursor_x, cursor_y;
    int    pet_x, pet_y;
    double cursor_distance;
    double cursor_idle_t;       /* Tempo sem o cursor se mover significativamente */
    int    last_cursor_x, last_cursor_y;

    /* ── Cliques ── */
    double last_click_t;
    int    click_burst_count;
    double click_interval;

    /* ── Drives interoceptivos (da homeostase) ── */
    double drive_energy;
    double drive_stress;
    double drive_curiosity;
    double drive_satiety;
    double drive_affection;
    double reward_trace;
    double punish_trace;

    /* ── Tempo acumulado ── */
    double total_time;
};

Sensors *sensors_open(void) {
    Sensors *s = calloc(1, sizeof(*s));
    if (s) {
        s->last_input_t    = 0.0;
        s->session_start_t = 0.0;
        s->drive_energy    = 0.85;
        s->drive_stress    = 0.15;
        s->drive_curiosity = 0.60;
        s->drive_satiety   = 0.75;
        s->drive_affection = 0.50;
    }
    return s;
}

void sensors_close(Sensors *s) {
    if (s) free(s);
}

void sensors_register_key(Sensors *s, char c, double now_t) {
    if (!s) return;
    double dt = now_t - s->last_input_t;
    s->key_press_interval = dt;
    s->last_input_t = now_t;
    s->last_key = c;

    if (dt < 0.20) {
        s->burst_counter++;
    } else {
        s->burst_counter = 1;
    }
}

void sensors_feed_physics(Sensors *s, double dt,
                          bool is_dragging, double vx, double vy, double speed,
                          bool clicked, bool bumped,
                          int cursor_x, int cursor_y, int pet_x, int pet_y) {
    if (!s) return;

    s->is_dragging = is_dragging;
    s->drag_speed  = speed;
    s->drag_vx     = vx;
    s->drag_vy     = vy;
    s->pet_x       = pet_x;
    s->pet_y       = pet_y;

    /* ── Detector de agitação (shake): inversões rápidas de vx ── */
    if (is_dragging) {
        s->shake_window_t += dt;

        if (fabs(vx) > 150.0) {
            if ((vx > 0 && s->last_drag_vx < -70.0) ||
                (vx < 0 && s->last_drag_vx >  70.0)) {
                s->direction_changes++;
            }
            s->last_drag_vx = vx;
        }

        /* Acumula intensidade de sacudida */
        if (s->direction_changes >= 3 && s->shake_window_t < 1.8) {
            s->shake_intensity = fmin(1.0, s->shake_intensity + 0.4 * dt * s->direction_changes);
        }

        /* Reset da janela de detecção */
        if (s->shake_window_t >= 1.8) {
            s->direction_changes = 0;
            s->shake_window_t = 0.0;
        }

        /* Arraste suave contínuo → carinho */
        if (speed > 4.0 && speed < 180.0) {
            s->gentle_drag_t += dt;
        } else {
            s->gentle_drag_t = fmax(0.0, s->gentle_drag_t - dt * 2.0);
        }
    } else {
        /* Fora do arraste: decai suavemente */
        s->shake_intensity = fmax(0.0, s->shake_intensity - 0.5 * dt);
        s->gentle_drag_t   = fmax(0.0, s->gentle_drag_t - dt * 3.0);
        s->direction_changes = 0;
        s->shake_window_t    = 0.0;
    }

    /* ── Proximidade e atividade do cursor ── */
    double cdist = hypot((double)(cursor_x - (pet_x + 96)),
                         (double)(cursor_y - (pet_y + 72)));
    s->cursor_distance = cdist;

    double cursor_moved = hypot((double)(cursor_x - s->last_cursor_x),
                                (double)(cursor_y - s->last_cursor_y));
    if (cursor_moved > 4.0) {
        s->cursor_idle_t = 0.0;
    } else {
        s->cursor_idle_t += dt;
    }
    s->last_cursor_x = cursor_x;
    s->last_cursor_y = cursor_y;

    /* ── Cliques ── */
    if (clicked) {
        double click_dt = s->total_time - s->last_click_t;
        s->click_interval = click_dt;
        s->last_click_t = s->total_time;

        if (click_dt < 0.35) {
            s->click_burst_count++;
        } else {
            s->click_burst_count = 1;
        }
        s->was_clicked = true;
    } else {
        s->was_clicked = false;
    }

    /* ── Impactos ── */
    s->was_bumped = bumped;

    s->total_time += dt;
}

void sensors_feed_drives(Sensors *s, const Drives *drives, double reward_trace, double punish_trace) {
    if (!s || !drives) return;
    s->drive_energy    = drives->energy;
    s->drive_stress    = drives->stress;
    s->drive_curiosity = drives->curiosity;
    s->drive_satiety   = drives->satiety;
    s->drive_affection = drives->affection;
    s->reward_trace    = reward_trace;
    s->punish_trace    = punish_trace;
}

void sensors_poll(Sensors *s, double now_t, SensorReading *reading, double inputs[PESTINHA_CHANNELS]) {
    if (!inputs) return;
    memset(inputs, 0, sizeof(double) * PESTINHA_CHANNELS);

    double idle = now_t - (s ? s->last_input_t : 0.0);
    if (reading) {
        reading->idle_time = idle;
    }

    if (!s) return;

    /* ═══════════════════════════════════════════════════════
     * BLOCO A: Canais Exteroceptivos (Interação de Hardware)
     * ═══════════════════════════════════════════════════════ */

    /* ── A.1: Digitação e Teclas (canais 0-15) ── */
    if (idle < 0.5) {
        inputs[CH_TYPING] = 0.85;
        if (s->key_press_interval < 0.15) {
            inputs[CH_KEY_FAST] = 0.90;
        } else {
            inputs[CH_KEY_SLOW] = 0.60;
        }
    }

    /* Mapeia teclas específicas nos seus canais */
    if (s->last_key != 0 && idle < 0.3) {
        switch (s->last_key) {
        case '\n':  inputs[CH_CMD_ENTER] = 0.90; break;
        case ' ':   inputs[CH_SPACE]     = 0.80; break;
        case (char)0x80: case (char)0x81:
        case (char)0x82: case (char)0x83:
                    inputs[CH_ARROW]     = 0.75; break;
        default:
            if (s->last_key >= 'a' && s->last_key <= 'z')
                inputs[CH_ALPHANUM] = 0.70;
            else if (s->last_key >= '0' && s->last_key <= '9')
                inputs[CH_NUMERIC] = 0.70;
            break;
        }
    }

    if (s->burst_counter > 5) {
        inputs[CH_BURST_INPUT] = fmin(1.0, 0.5 + s->burst_counter * 0.05);
    }

    /* ── A.2: Gestos Físicos de Mouse/Arraste (canais 16-31) ── */

    /* Carinho: arraste suave contínuo */
    if (s->gentle_drag_t > 0.25) {
        double caress_strength = fmin(1.0, s->gentle_drag_t / 1.5);
        inputs[CH_CARESS] = caress_strength;
        inputs[CH_OXYTOCIN] = caress_strength * 0.7;
    }

    /* Cutucada: clique direto ou impacto de borda */
    if (s->was_clicked) {
        inputs[CH_POKE] = 0.75;
        if (s->click_burst_count >= 3) {
            inputs[CH_POKE] = 0.95;  /* Cliques em rajada = cutucada forte */
        }
    }
    if (s->was_bumped) {
        inputs[CH_POKE]  = fmax(inputs[CH_POKE], 0.85);
        inputs[CH_NOISE] = 0.80;
    }

    /* Sacudida / agitação violenta */
    if (s->shake_intensity > 0.1) {
        inputs[CH_NOISE]   = fmax(inputs[CH_NOISE], s->shake_intensity);
        inputs[CH_PUNISH]  = s->shake_intensity * 0.6;
        inputs[CH_ADRENALINE] = s->shake_intensity * 0.8;
    }

    /* Teclas de interação mapeadas como sensores específicos */
    if (s->last_key == 'c' || s->last_key == 'C' || s->last_key == '\n') {
        if (idle < 0.3) {
            inputs[CH_CARESS] = fmax(inputs[CH_CARESS], 0.90);
            inputs[CH_REWARD] = 0.80;
        }
    }
    if (s->last_key == 'p' || s->last_key == 'P') {
        if (idle < 0.3) {
            inputs[CH_POKE]   = fmax(inputs[CH_POKE], 0.85);
            inputs[CH_PUNISH] = fmax(inputs[CH_PUNISH], 0.60);
        }
    }
    if (s->last_key == 'f' || s->last_key == 'F') {
        if (idle < 0.3) {
            inputs[CH_FEED]   = 0.90;
            inputs[CH_REWARD] = fmax(inputs[CH_REWARD], 0.70);
        }
    }
    if (s->last_key == 't' || s->last_key == 'T') {
        if (idle < 0.3) {
            inputs[CH_TALK] = 0.80;
            inputs[CH_GREET] = 0.60;
        }
    }

    /* Cliques espaçados vs rápidos */
    if (s->was_clicked) {
        if (s->click_interval > 0.8) {
            inputs[CH_SLOW_CLICK] = 0.75;
        } else {
            inputs[CH_QUICK_CLICK] = 0.80;
        }
    }

    /* Brincadeira: arraste rápido mas não violento (speed 180..500) */
    if (s->is_dragging && s->drag_speed > 180.0 && s->drag_speed < 500.0 && s->shake_intensity < 0.3) {
        inputs[CH_PLAY] = fmin(1.0, (s->drag_speed - 180.0) / 320.0);
    }

    /* ── A.3: Presença e Ausência (canais 3, 21, 36, 42-43) ── */
    if (idle > 5.0) {
        inputs[CH_IDLE] = fmin(1.0, (idle - 5.0) / 10.0);
    }
    if (idle > 30.0) {
        inputs[CH_IDLE_LONG] = fmin(1.0, (idle - 30.0) / 60.0);
        inputs[CH_IGNORE] = fmin(1.0, (idle - 30.0) / 120.0);
    }

    /* Proximidade do cursor ao pet */
    if (s->cursor_distance < 300.0 && s->cursor_idle_t < 2.0) {
        inputs[CH_USER_PRESENT] = fmin(1.0, 1.0 - s->cursor_distance / 300.0);
        inputs[CH_FOCUS_ACTIVE] = inputs[CH_USER_PRESENT] * 0.8;
    } else if (s->cursor_idle_t > 5.0) {
        inputs[CH_USER_AWAY] = fmin(1.0, s->cursor_idle_t / 15.0);
    }

    /* ═══════════════════════════════════════════════════════
     * BLOCO B: Canais Temporais e Circadianos (canais 32-39)
     * ═══════════════════════════════════════════════════════ */
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo) {
        int hour = timeinfo->tm_hour;
        if (hour >= 6 && hour < 18) {
            inputs[CH_TIME_DAY]      = 0.70;
            inputs[CH_CIRCADIAN_PEAK] = 0.80;
        } else {
            inputs[CH_TIME_NIGHT]    = 0.85;
            inputs[CH_CIRCADIAN_LOW] = 0.80;
        }
    }

    /* Uptime da sessão */
    inputs[CH_UPTIME] = fmin(1.0, s->total_time / 3600.0); /* satura em 1h */

    /* Duração da sessão */
    inputs[CH_SESSION_LEN] = fmin(1.0, s->total_time / 1800.0); /* satura em 30min */

    /* Pulso biológico do loop */
    inputs[CH_TICK_PULSE] = (sin(now_t * 3.14159) + 1.0) * 0.5;

    /* Ritmo regular de digitação */
    if (s->key_press_interval > 0.2 && s->key_press_interval < 0.8 && idle < 2.0) {
        inputs[CH_RHYTHM_REGULAR] = 0.70;
    }

    /* ═══════════════════════════════════════════════════════
     * BLOCO C: Canais Interoceptivos / Drives (canais 48-63)
     * ═══════════════════════════════════════════════════════ */

    /* Fadiga e fome como pulsos sensoriais internos */
    if (s->drive_energy < 0.30) {
        inputs[CH_FATIGUE_PULSE] = 1.0 - s->drive_energy / 0.30;
        inputs[CH_SLEEP_REQ]     = inputs[CH_FATIGUE_PULSE] * 0.7;
    }
    if (s->drive_satiety < 0.30) {
        inputs[CH_HUNGER_PULSE] = 1.0 - s->drive_satiety / 0.30;
    }

    /* Batimento cardíaco baseado no arousal (estresse como proxy) */
    if (s->drive_stress > 0.5) {
        inputs[CH_HEARTBEAT_FAST] = (s->drive_stress - 0.5) * 2.0;
        inputs[CH_ADRENALINE] = fmax(inputs[CH_ADRENALINE], s->drive_stress * 0.6);
        inputs[CH_CORTISOL]   = fmax(inputs[CH_CORTISOL], (s->drive_stress - 0.5) * 1.5);
    } else {
        inputs[CH_HEARTBEAT_SLOW] = 1.0 - s->drive_stress * 2.0;
    }

    /* Respiração */
    inputs[CH_BREATHING] = (sin(now_t * 0.8) + 1.0) * 0.5;

    /* Neuromoduladores baseados nos traces de conditioning */
    if (s->reward_trace > 0.05) {
        inputs[CH_DOPAMINE]  = fmax(inputs[CH_DOPAMINE], fmin(1.0, s->reward_trace));
        inputs[CH_ENDORPHIN] = fmin(1.0, s->reward_trace * 0.8);
        inputs[CH_REWARD]    = fmax(inputs[CH_REWARD], s->reward_trace);
    }
    if (s->punish_trace > 0.05) {
        inputs[CH_CORTISOL]  = fmax(inputs[CH_CORTISOL], fmin(1.0, s->punish_trace));
        inputs[CH_PUNISH]    = fmax(inputs[CH_PUNISH], s->punish_trace);
    }

    /* Serotonina: inversamente correlacionada ao estresse */
    inputs[CH_SEROTONIN] = fmax(0.0, 1.0 - s->drive_stress * 1.5);

    /* Oxitocina: proporcional à afeição */
    inputs[CH_OXYTOCIN] = fmax(inputs[CH_OXYTOCIN], s->drive_affection * 0.8);

    /* Traço de memória e STDP: proporcionais à atividade recente */
    if (idle < 1.0) {
        inputs[CH_MEMORY_TRACE] = 0.5;
        inputs[CH_STDP_SPIKE]   = 0.4;
    }

    /* Expectativa de recompensa */
    if (s->reward_trace > 0.1 && s->drive_curiosity > 0.5) {
        inputs[CH_REWARD_WAIT] = fmin(1.0, s->reward_trace * s->drive_curiosity * 2.0);
    }
}

/* behavior_dmn.h — Default Mode Network (rede de modo padrão)
 * Ativa comportamentos espontâneos quando o pet está ocioso.
 * Modela o "pensamento à toa" biológico.
 */
#ifndef BEHAVIOR_DMN_H
#define BEHAVIOR_DMN_H

#include <stdbool.h>
#include "../core/fixed.h"

typedef enum {
    DMN_IDLE = 0,       /* sem comportamento DMN ativo */
    DMN_DAYDREAM,       /* olhando para o nada (piscar lento, olhar vago) */
    DMN_GROOMING,       /* auto-cuidado: coçar, esticar */
    DMN_YAWN,           /* bocejo emergente */
    DMN_LOOK_AROUND,    /* olhar escandaloso para os lados */
    DMN_MICRO_CHIRP,    /* som baixinho espontâneo */
    DMN_STATE_COUNT
} DmnState;

typedef struct {
    DmnState  state;
    double    idle_since;       /* tempo da última interação */
    double    state_start;      /* quando o estado DMN atual começou */
    double    state_duration;   /* duração planejada do estado (seg) */
    fix16_t   arousal;          /* 0=dormindo..1=ativo */
    int       greet_count;      /* quantas vezes o pet "chamou atenção" hoje */
    double    last_greet;       /* timestamp do último chamado */
} BehaviorDmn;

void     behavior_dmn_init(BehaviorDmn *d);
/* Alimenta o DMN com o timestamp atual e o nível de excitação.
 * idle_threshold: segundos de inatividade para ativar o DMN (ex: 15.0).
 * Retorna o estado DMN ativo (pode ser DMN_IDLE). */
DmnState behavior_dmn_update(BehaviorDmn *d, double now_sec,
                              fix16_t arousal, double idle_threshold);

/* Chama quando qualquer entrada do usuário ocorrer (reseta idle timer). */
void     behavior_dmn_signal_activity(BehaviorDmn *d, double now_sec);

/* Retorna true se o DMN quer emitir um "oi, olha pra mim!" */
bool     behavior_dmn_wants_attention(const BehaviorDmn *d);

/* Nome legível do estado */
const char *behavior_dmn_state_name(DmnState s);

#endif /* BEHAVIOR_DMN_H */

/* ontogeny.h — Ontogenia do Caddin (C11, sem float, sem malloc).
 *
 * Q8.8 = int16_t com 8 bits fracionários (256 == 1.0), como em fixed.h.
 * Aqui os tipos são standalone; troque q88_t por fix16_t ao integrar.
 *
 * Peças:
 *   1. Janela crítica: `brake` por pool (Q0.32) escala a taxa de aprendizado.
 *   2. Estágios (filhote/juvenil/adulto/veterano) com histerese.
 *   3. Cristalização por projeção: PLASTIC -> CANDIDATE -> CRYSTAL, com
 *      validação externa (quantização + equivalência) e descongelamento.
 *   4. Imprinting do dono com confirmação (sessões + sono).
 *   5. Formato caddin.brain v3 (seções com CRC32 individual).
 */
#ifndef ONTOGENY_H
#define ONTOGENY_H

#include <stdint.h>
#include <stddef.h>

typedef int16_t q88_t;
#define Q88_ONE 256

#define ONTO_POOLS    120   /* 64 sensoriais + 32 associativos + 24 motores */
#define ONTO_MAX_PROJ 2048

typedef enum { ONTO_FILHOTE = 0, ONTO_JUVENIL, ONTO_ADULTO, ONTO_VETERANO } OntoStage;
typedef enum { PROJ_PLASTIC = 0, PROJ_CANDIDATE, PROJ_CRYSTAL } ProjState;

/* ---------------------------------------------------------------- params */
typedef struct {
    uint32_t target_s;        /* s de domínio pleno p/ fechar a janela (>= 1000) */
    q88_t    theta_lo;        /* err_ema < theta_lo  => dominou (fecha janela)   */
    q88_t    theta_hi;        /* err_ema > theta_hi  => reabre a janela          */
    q88_t    act_min;         /* atividade mínima p/ contar como exposição       */
    q88_t    lr_floor;        /* piso da taxa de aprendizado (adulto ainda aprende) */
    uint32_t eps_dw;          /* |dw| médio/sinapse/ciclo máx., em 1/256 LSB     */
    uint32_t min_spikes;      /* uso mínimo por ciclo de sono                    */
    uint32_t min_credit;      /* crédito dopaminérgico mínimo por ciclo          */
    uint8_t  n_stab;          /* ciclos de sono estáveis p/ virar candidata      */
    uint8_t  cooldown_sleeps; /* quarentena após falha/descongelamento           */
    uint16_t brake_min_q16;   /* brake mínimo (Q0.16) dos 2 pools p/ cristalizar */
    q88_t    err_melt;        /* err_ema do pool pós acima disto conta p/ melt   */
    q88_t    ne_melt;         /* noradrenalina mínima p/ descongelar             */
    uint16_t melt_ticks;      /* ticks (1 Hz) seguidos p/ descongelar            */
} OntoParams;

OntoParams onto_params_default(void);

/* ---------------------------------------------------------------- estado */
typedef struct {
    uint32_t brake;           /* Q0.32: 0 = janela aberta, 0xFFFFFFFF = fechada  */
    q88_t    err_ema;         /* mismatch preditivo suavizado (Q8.8, 0..256)     */
    q88_t    act_ema;         /* fração de atividade suavizada (Q8.8, 0..256)    */
    uint8_t  seen;            /* já teve exposição real (para o cálculo de estágio) */
} OntoPool;

typedef struct {
    uint8_t  pre, post;       /* ids de pool */
    uint8_t  state;           /* ProjState */
    uint8_t  stab;            /* ciclos de sono estáveis consecutivos */
    uint8_t  cooldown;
    uint8_t  restore;         /* 1 = engine deve reconstituir pesos plásticos */
    uint16_t melt_cnt;
    uint16_t n_syn;
    uint32_t acc_abs_dw;      /* soma |dw| no ciclo (LSB Q8.8) — a plasticidade soma */
    uint32_t acc_spikes;      /* spikes que atravessaram no ciclo                 */
    uint32_t acc_credit;      /* sum(spikes * DA_fásica) >> 8                     */
} OntoProj;

typedef struct {
    OntoParams prm;
    uint8_t    stage;
    uint32_t   awake_s, sleeps;
    uint16_t   n_proj;
    OntoPool   pool[ONTO_POOLS];
    OntoProj   proj[ONTO_MAX_PROJ];
} Ontogeny;

void  onto_init(Ontogeny *o, const OntoParams *prm);          /* prm NULL => default */
void  onto_init_migrated(Ontogeny *o, const OntoParams *prm); /* cérebro v2: brake=50% */
int   onto_add_proj(Ontogeny *o, uint8_t pre, uint8_t post, uint16_t n_syn); /* idx ou -1 */

/* Multiplicador de aprendizado do pool, Q8.8 em [lr_floor, 256].
 * Use: dw = (dw_raw * onto_lr_scale(o, pool)) >> 8.  Projeção CRYSTAL: dw = 0. */
q88_t onto_lr_scale(const Ontogeny *o, int pool);

/* 1 Hz, acordado. err/act: Q8.8 [0,256] por pool. ne: noradrenalina Q8.8. */
void  onto_tick_1hz(Ontogeny *o, const q88_t err[ONTO_POOLS],
                    const q88_t act[ONTO_POOLS], q88_t ne);

/* A plasticidade/propagação chama isto 1x/s por projeção (ou agrega). da >= 0. */
void  onto_proj_note(OntoProj *p, uint32_t spikes, uint32_t abs_dw, q88_t da);

/* No sono. Retorna quantas projeções viraram CANDIDATE neste ciclo. */
int   onto_sleep_consolidate(Ontogeny *o);

/* A engine valida a candidata (quantização + equivalência) e confirma.
 * ok=1 -> CRYSTAL ; ok=0 -> volta a PLASTIC com quarentena (rollback). */
void  onto_commit(Ontogeny *o, int proj, int ok);

int   onto_count_state(const Ontogeny *o, ProjState s);

/* ------------------------------------------------- cristal (bloco int8) */
/* Quantiza n pesos Q8.8 -> int8 com UMA escala por bloco (em LSB Q8.8).
 * Retorna 1 se max_err <= tol (bloco aceitável), 0 caso contrário. */
int  crystal_quantize(const q88_t *w, int n, int8_t *q, uint16_t *scale,
                      q88_t *max_err, q88_t tol);
void crystal_dequantize(const int8_t *q, int n, uint16_t scale, q88_t *w);
/* acc[j] += sum_{i em spikes} q[i*n_post+j]*scale  (n_pre <= 64) */
void crystal_propagate(const int8_t *q, uint16_t scale, int n_pre, int n_post,
                       uint64_t spikes, int32_t *acc);
/* Equivalência: saídas motoras de replay (ref x cristal) dentro de tol. */
int  onto_equiv_check(const q88_t *ref, const q88_t *got, int n, q88_t tol);
int  onto_equiv_spikes(uint32_t ref_cnt, uint32_t got_cnt, uint32_t tol_permille);

/* ------------------------------------------------------------- imprint */
typedef struct {
    uint8_t state;            /* 0 vazio, 1 candidato, 2 imprintado */
    uint8_t sessions, sleeps;
    q88_t   timbre[8];        /* envelope espectral em 8 bandas (Q8.8, 0..256) */
} Imprint;

#define IMPRINT_MATCH_MIN 192 /* 0.75 */
q88_t imprint_match(const Imprint *im, const q88_t timbre[8]); /* 256 = idêntico */
void  imprint_observe(Imprint *im, const q88_t timbre[8], int new_session);
void  imprint_sleep(Imprint *im);   /* confirma: >=3 sessões e >=1 sono */
void  imprint_reset(Imprint *im);   /* ritual de adoção */

/* ------------------------------------------------------ caddin.brain v3
 * Cabeçalho (20 B, little-endian):
 *   0  u32 magic 0x43414444 ("CADD")   4 u16 versão=3   6 u16 flags
 *   8  u32 total_len                  12 u32 n_sections 16 u32 crc32(0..15)
 * Seção: u32 tag | u32 len | u32 crc32(payload) | payload[len]
 * Tags: 'SNN ' (blob v2 opaco, obrigatório), 'ONTO', 'IMPR', 'CRYS' (opaco).
 * Tags desconhecidas são puladas (compatibilidade futura).
 * Falha em ONTO/IMPR/CRYS é "suave" (status 2); falha em SNN é fatal.   */
#define BRAIN3_MAGIC   0x43414444u
#define BRAIN3_VERSION 3
#define BRAIN3_TAG(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))
#define TAG_SNN  BRAIN3_TAG('S','N','N',' ')
#define TAG_ONTO BRAIN3_TAG('O','N','T','O')
#define TAG_IMPR BRAIN3_TAG('I','M','P','R')
#define TAG_CRYS BRAIN3_TAG('C','R','Y','S')

typedef struct { const uint8_t *ptr; uint32_t len; int status; } Brain3Sec; /* 0 ausente, 1 ok, 2 corrompida */
typedef struct { Brain3Sec snn, onto, impr, crys; uint32_t n_unknown; } Brain3Index;

uint32_t brain3_crc32(const void *p, size_t n);   /* troque pelo CRC do persist.c */
size_t brain3_begin(uint8_t *buf, size_t cap);    /* retorna offset (20) ou 0 */
size_t brain3_put(uint8_t *buf, size_t cap, size_t off, uint32_t tag,
                  const void *p, uint32_t len);   /* novo offset ou 0 (sem espaço) */
void   brain3_finish(uint8_t *buf, size_t total, uint32_t n_sections);
/* 0 ok | -1 cabeçalho/magic/versão | -2 tamanho/truncado | -3 seção SNN ausente/corrompida */
int    brain3_parse(const uint8_t *buf, size_t len, Brain3Index *idx);
/* Migração v2 -> v3: embrulha o arquivo v2 inteiro como seção 'SNN '. */
size_t brain3_wrap_v2(uint8_t *buf, size_t cap, const uint8_t *v2, size_t v2len);

size_t onto_serialize(const Ontogeny *o, uint8_t *out, size_t cap);   /* 0 se cap curto */
int    onto_deserialize(Ontogeny *o, const uint8_t *in, size_t len);  /* 1 ok; 0 => o resetado */
size_t imprint_serialize(const Imprint *im, uint8_t *out, size_t cap);
int    imprint_deserialize(Imprint *im, const uint8_t *in, size_t len);

#endif /* ONTOGENY_H */

/* ontogeny.c — ver ontogeny.h. Aritmética inteira; sem float, sem malloc. */
#include "ontogeny.h"
#include <string.h>

/* ------------------------------------------------------------ utilidades */
static inline int clamp_q88(int v) { return v < 0 ? 0 : (v > Q88_ONE ? Q88_ONE : v); }

/* EMA com janela ~8, arredondamento simétrico (sem viés de piso). */
static inline int ema8(int x, int target)
{
    int d = target - x;
    return x + (d >= 0 ? (d + 4) / 8 : -((-d + 4) / 8));
}

static inline uint32_t sat_add32(uint32_t a, uint32_t b)
{
    uint32_t s = a + b;
    return s < a ? UINT32_MAX : s;
}

static inline void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static inline void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline uint16_t get_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ---------------------------------------------------------------- params */
OntoParams onto_params_default(void)
{
    OntoParams p = {
        .target_s = 72000,      /* ~20 h acordado de domínio pleno */
        .theta_lo = 38,         /* 0.15 */
        .theta_hi = 128,        /* 0.50 */
        .act_min = 26,          /* 0.10 */
        .lr_floor = 13,         /* ~5%: adulto ainda aprende um pouco */
        .eps_dw = 64,           /* 0.25 LSB por sinapse por ciclo */
        .min_spikes = 1000,
        .min_credit = 100,
        .n_stab = 5,
        .cooldown_sleeps = 3,
        .brake_min_q16 = 39321, /* 60% */
        .err_melt = 128,
        .ne_melt = 128,
        .melt_ticks = 600       /* 10 min */
    };
    return p;
}

void onto_init(Ontogeny *o, const OntoParams *prm)
{
    OntoParams p = prm ? *prm : onto_params_default();
    if (p.target_s < 1000) p.target_s = 1000;   /* garante aritmética em 32 bits */
    memset(o, 0, sizeof *o);
    o->prm = p;
    o->stage = ONTO_FILHOTE;
}

void onto_init_migrated(Ontogeny *o, const OntoParams *prm)
{
    onto_init(o, prm);
    for (int i = 0; i < ONTO_POOLS; i++) { o->pool[i].brake = 0x80000000u; o->pool[i].seen = 1; }
    o->stage = ONTO_JUVENIL;
}

int onto_add_proj(Ontogeny *o, uint8_t pre, uint8_t post, uint16_t n_syn)
{
    if (o->n_proj >= ONTO_MAX_PROJ || pre >= ONTO_POOLS || post >= ONTO_POOLS) return -1;
    OntoProj *p = &o->proj[o->n_proj];
    memset(p, 0, sizeof *p);
    p->pre = pre; p->post = post; p->n_syn = n_syn;
    return o->n_proj++;
}

/* ----------------------------------------------------------------- brake */
q88_t onto_lr_scale(const Ontogeny *o, int pool)
{
    int s = Q88_ONE - (int)(o->pool[pool].brake >> 24);
    return (q88_t)(s < o->prm.lr_floor ? o->prm.lr_floor : s);
}

static void stage_update(Ontogeny *o)
{
    static const uint16_t up[3] = { 16384, 39321, 58982 };   /* 25% 60% 90% */
    const uint16_t hy = 3277;                                /* 5% de histerese */
    uint32_t sum = 0, n = 0;
    for (int i = 0; i < ONTO_POOLS; i++)
        if (o->pool[i].seen) { sum += o->pool[i].brake >> 16; n++; }
    if (n < ONTO_POOLS / 4) return;                          /* poucos sentidos ativos */
    uint32_t mean = sum / n;                                 /* Q0.16 */
    int s = o->stage;
    while (s < ONTO_VETERANO && mean >= up[s]) s++;
    while (s > ONTO_FILHOTE && mean + hy < up[s - 1]) s--;
    o->stage = (uint8_t)s;
}

void onto_tick_1hz(Ontogeny *o, const q88_t err[ONTO_POOLS],
                   const q88_t act[ONTO_POOLS], q88_t ne)
{
    const OntoParams *P = &o->prm;
    const uint32_t rate = UINT32_MAX / P->target_s;          /* <= 2^22 */

    for (int i = 0; i < ONTO_POOLS; i++) {
        OntoPool *pl = &o->pool[i];
        pl->err_ema = (q88_t)ema8(pl->err_ema, clamp_q88(err[i]));
        pl->act_ema = (q88_t)ema8(pl->act_ema, clamp_q88(act[i]));
        if (pl->act_ema < P->act_min) continue;              /* privação: brake congelado */
        pl->seen = 1;
        if (pl->err_ema < P->theta_lo) {                     /* domínio: fecha a janela   */
            uint32_t m = ((uint32_t)(P->theta_lo - pl->err_ema) << 8) / (uint32_t)P->theta_lo; /* 0..256 */
            uint32_t t = (rate * m) >> 8;                    /* <= 2^30, cabe em 32 bits  */
            uint32_t d = (t * (uint32_t)pl->act_ema) >> 8;   /* enriquecimento acelera    */
            pl->brake = sat_add32(pl->brake, d);
        } else if (pl->err_ema > P->theta_hi) {              /* surpresa crônica: reabre  */
            uint32_t d = rate >> 2;                          /* 4x mais devagar que fechar */
            pl->brake = pl->brake > d ? pl->brake - d : 0;
        }                                                    /* entre os limiares: histerese */
    }

    for (int k = 0; k < o->n_proj; k++) {                    /* descongelamento */
        OntoProj *p = &o->proj[k];
        if (p->state != PROJ_CRYSTAL) continue;
        if (o->pool[p->post].err_ema > P->err_melt && ne >= P->ne_melt) {
            if (p->melt_cnt < 0xFFFF) p->melt_cnt++;
        } else if (p->melt_cnt) {
            p->melt_cnt--;
        }
        if (p->melt_cnt >= P->melt_ticks) {
            p->state = PROJ_PLASTIC; p->stab = 0; p->melt_cnt = 0;
            p->cooldown = P->cooldown_sleeps; p->restore = 1;
        }
    }

    o->awake_s++;
    stage_update(o);
}

/* ---------------------------------------------------------- cristalização */
void onto_proj_note(OntoProj *p, uint32_t spikes, uint32_t abs_dw, q88_t da)
{
    p->acc_spikes  = sat_add32(p->acc_spikes, spikes);
    p->acc_abs_dw  = sat_add32(p->acc_abs_dw, abs_dw);
    if (da > 0) p->acc_credit = sat_add32(p->acc_credit, (uint32_t)((spikes * (uint32_t)da) >> 8));
}

int onto_sleep_consolidate(Ontogeny *o)
{
    const OntoParams *P = &o->prm;
    int cand = 0;
    for (int k = 0; k < o->n_proj; k++) {
        OntoProj *p = &o->proj[k];
        if (p->state == PROJ_PLASTIC) {
            if (p->cooldown) {
                p->cooldown--;
            } else {
                int stable = p->acc_abs_dw <= ((uint32_t)P->eps_dw * p->n_syn) >> 8;
                int used   = p->acc_spikes >= P->min_spikes;
                int valued = p->acc_credit >= P->min_credit;
                if (!stable)                 p->stab = 0;          /* instável: recomeça    */
                else if (used && valued && p->stab < 255) p->stab++; /* estável+útil: avança */
                int brake_ok = (o->pool[p->pre].brake >> 16)  >= P->brake_min_q16 &&
                               (o->pool[p->post].brake >> 16) >= P->brake_min_q16;
                if (p->stab >= P->n_stab && brake_ok) { p->state = PROJ_CANDIDATE; cand++; }
            }
        }
        p->acc_abs_dw = p->acc_spikes = p->acc_credit = 0;
    }
    o->sleeps++;
    return cand;
}

void onto_commit(Ontogeny *o, int proj, int ok)
{
    if (proj < 0 || proj >= o->n_proj || o->proj[proj].state != PROJ_CANDIDATE) return;
    OntoProj *p = &o->proj[proj];
    if (ok) { p->state = PROJ_CRYSTAL; p->melt_cnt = 0; }
    else    { p->state = PROJ_PLASTIC; p->stab = 0; p->cooldown = o->prm.cooldown_sleeps; }
}

int onto_count_state(const Ontogeny *o, ProjState s)
{
    int n = 0;
    for (int k = 0; k < o->n_proj; k++) n += (o->proj[k].state == s);
    return n;
}

/* ----------------------------------------------------------- bloco int8 */
int crystal_quantize(const q88_t *w, int n, int8_t *q, uint16_t *scale,
                     q88_t *max_err, q88_t tol)
{
    int32_t maxabs = 0;
    for (int i = 0; i < n; i++) { int32_t a = w[i] < 0 ? -(int32_t)w[i] : w[i]; if (a > maxabs) maxabs = a; }
    int32_t sc = maxabs ? (maxabs + 126) / 127 : 1;           /* ceil(maxabs/127) */
    int32_t me = 0;
    for (int i = 0; i < n; i++) {
        int32_t v = w[i];
        int32_t r = v >= 0 ? (v + sc / 2) / sc : -((-v + sc / 2) / sc);
        if (r > 127) r = 127; else if (r < -127) r = -127;
        q[i] = (int8_t)r;
        int32_t e = v - r * sc; if (e < 0) e = -e;
        if (e > me) me = e;
    }
    *scale = (uint16_t)sc;
    *max_err = (q88_t)(me > 32767 ? 32767 : me);
    return me <= tol;
}

void crystal_dequantize(const int8_t *q, int n, uint16_t scale, q88_t *w)
{
    for (int i = 0; i < n; i++) w[i] = (q88_t)((int32_t)q[i] * scale);
}

static inline int ctz64(uint64_t x)
{
#if defined(__GNUC__)
    return __builtin_ctzll(x);
#else
    int n = 0; while (!(x & 1)) { x >>= 1; n++; } return n;
#endif
}

void crystal_propagate(const int8_t *q, uint16_t scale, int n_pre, int n_post,
                       uint64_t spikes, int32_t *acc)
{
    if (n_pre < 64) spikes &= (1ULL << n_pre) - 1ULL;
    while (spikes) {
        int i = ctz64(spikes);
        spikes &= spikes - 1;
        const int8_t *row = q + (size_t)i * n_post;
        for (int j = 0; j < n_post; j++) acc[j] += (int32_t)row[j] * scale;
    }
}

int onto_equiv_check(const q88_t *ref, const q88_t *got, int n, q88_t tol)
{
    for (int i = 0; i < n; i++) {
        int d = ref[i] - got[i]; if (d < 0) d = -d;
        if (d > tol) return 0;
    }
    return 1;
}

int onto_equiv_spikes(uint32_t ref_cnt, uint32_t got_cnt, uint32_t tol_permille)
{
    uint32_t d = ref_cnt > got_cnt ? ref_cnt - got_cnt : got_cnt - ref_cnt;
    uint32_t base = ref_cnt ? ref_cnt : 1;
    return (uint64_t)d * 1000u <= (uint64_t)tol_permille * base;
}

/* --------------------------------------------------------------- imprint */
q88_t imprint_match(const Imprint *im, const q88_t timbre[8])
{
    int sum = 0;
    for (int i = 0; i < 8; i++) { int d = im->timbre[i] - timbre[i]; sum += d < 0 ? -d : d; }
    int s = Q88_ONE - sum / 8;
    return (q88_t)(s < 0 ? 0 : s);
}

void imprint_observe(Imprint *im, const q88_t timbre[8], int new_session)
{
    if (im->state == 2) return;                               /* já imprintado */
    if (im->state == 0) {
        memcpy(im->timbre, timbre, sizeof im->timbre);
        im->state = 1; im->sessions = 1; im->sleeps = 0;
        return;
    }
    if (imprint_match(im, timbre) < IMPRINT_MATCH_MIN) return; /* estranho: ignorado */
    for (int i = 0; i < 8; i++) im->timbre[i] = (q88_t)ema8(im->timbre[i], timbre[i]);
    if (new_session && im->sessions < 255) im->sessions++;
}

void imprint_sleep(Imprint *im)
{
    if (im->state != 1) return;
    if (im->sleeps < 255) im->sleeps++;
    if (im->sessions >= 3 && im->sleeps >= 1) im->state = 2;
}

void imprint_reset(Imprint *im) { memset(im, 0, sizeof *im); }

/* ----------------------------------------------------------- serialização */
#define ONTO_HDR  12
#define ONTO_POOL_B 9
#define ONTO_PROJ_B 9

size_t onto_serialize(const Ontogeny *o, uint8_t *out, size_t cap)
{
    size_t need = ONTO_HDR + (size_t)ONTO_POOLS * ONTO_POOL_B + (size_t)o->n_proj * ONTO_PROJ_B;
    if (cap < need) return 0;
    uint8_t *p = out;
    p[0] = o->stage; p[1] = 0; put_u16(p + 2, o->n_proj);
    put_u32(p + 4, o->awake_s); put_u32(p + 8, o->sleeps);
    p += ONTO_HDR;
    for (int i = 0; i < ONTO_POOLS; i++, p += ONTO_POOL_B) {
        put_u32(p, o->pool[i].brake);
        put_u16(p + 4, (uint16_t)o->pool[i].err_ema);
        put_u16(p + 6, (uint16_t)o->pool[i].act_ema);
        p[8] = o->pool[i].seen;
    }
    for (int k = 0; k < o->n_proj; k++, p += ONTO_PROJ_B) {
        const OntoProj *q = &o->proj[k];
        p[0] = q->pre; p[1] = q->post; p[2] = q->state; p[3] = q->stab; p[4] = q->cooldown;
        put_u16(p + 5, q->melt_cnt); put_u16(p + 7, q->n_syn);
    }
    return need;
}

int onto_deserialize(Ontogeny *o, const uint8_t *in, size_t len)
{
    OntoParams keep = o->prm;                 /* parâmetros não são persistidos */
    onto_init(o, &keep);
    if (len < ONTO_HDR) return 0;
    uint16_t np = get_u16(in + 2);
    if (in[0] > ONTO_VETERANO || np > ONTO_MAX_PROJ) return 0;
    if (len != ONTO_HDR + (size_t)ONTO_POOLS * ONTO_POOL_B + (size_t)np * ONTO_PROJ_B) return 0;
    o->stage = in[0]; o->n_proj = np;
    o->awake_s = get_u32(in + 4); o->sleeps = get_u32(in + 8);
    const uint8_t *p = in + ONTO_HDR;
    for (int i = 0; i < ONTO_POOLS; i++, p += ONTO_POOL_B) {
        o->pool[i].brake = get_u32(p);
        o->pool[i].err_ema = (q88_t)clamp_q88((int16_t)get_u16(p + 4));
        o->pool[i].act_ema = (q88_t)clamp_q88((int16_t)get_u16(p + 6));
        o->pool[i].seen = p[8] ? 1 : 0;
    }
    for (int k = 0; k < np; k++, p += ONTO_PROJ_B) {
        OntoProj *q = &o->proj[k];
        q->pre = p[0]; q->post = p[1]; q->state = p[2]; q->stab = p[3]; q->cooldown = p[4];
        q->melt_cnt = get_u16(p + 5); q->n_syn = get_u16(p + 7);
        if (q->pre >= ONTO_POOLS || q->post >= ONTO_POOLS || q->state > PROJ_CRYSTAL) {
            onto_init(o, &keep); return 0;
        }
        if (q->state == PROJ_CANDIDATE) q->state = PROJ_PLASTIC;  /* candidata não sobrevive a reboot */
    }
    return 1;
}

size_t imprint_serialize(const Imprint *im, uint8_t *out, size_t cap)
{
    if (cap < 19) return 0;
    out[0] = im->state; out[1] = im->sessions; out[2] = im->sleeps;
    for (int i = 0; i < 8; i++) put_u16(out + 3 + 2 * i, (uint16_t)im->timbre[i]);
    return 19;
}

int imprint_deserialize(Imprint *im, const uint8_t *in, size_t len)
{
    memset(im, 0, sizeof *im);
    if (len != 19 || in[0] > 2) return 0;
    im->state = in[0]; im->sessions = in[1]; im->sleeps = in[2];
    for (int i = 0; i < 8; i++) im->timbre[i] = (q88_t)clamp_q88((int16_t)get_u16(in + 3 + 2 * i));
    return 1;
}

/* ------------------------------------------------------- caddin.brain v3 */
uint32_t brain3_crc32(const void *p, size_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *b++;
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}

size_t brain3_begin(uint8_t *buf, size_t cap)
{
    if (cap < 20) return 0;
    memset(buf, 0, 20);
    return 20;
}

size_t brain3_put(uint8_t *buf, size_t cap, size_t off, uint32_t tag, const void *p, uint32_t len)
{
    if (off < 20 || off > cap || cap - off < 12u + (size_t)len) return 0;
    put_u32(buf + off, tag); put_u32(buf + off + 4, len);
    put_u32(buf + off + 8, brain3_crc32(p, len));
    memcpy(buf + off + 12, p, len);
    return off + 12 + len;
}

void brain3_finish(uint8_t *buf, size_t total, uint32_t n_sections)
{
    put_u32(buf, BRAIN3_MAGIC); put_u16(buf + 4, BRAIN3_VERSION); put_u16(buf + 6, 0);
    put_u32(buf + 8, (uint32_t)total); put_u32(buf + 12, n_sections);
    put_u32(buf + 16, brain3_crc32(buf, 16));
}

int brain3_parse(const uint8_t *buf, size_t len, Brain3Index *idx)
{
    memset(idx, 0, sizeof *idx);
    if (len < 20 || get_u32(buf) != BRAIN3_MAGIC || get_u16(buf + 4) != BRAIN3_VERSION ||
        get_u32(buf + 16) != brain3_crc32(buf, 16)) return -1;
    if (get_u32(buf + 8) != len) return -2;
    uint32_t n = get_u32(buf + 12);
    size_t off = 20;
    for (uint32_t s = 0; s < n; s++) {
        if (len - off < 12) return -2;
        uint32_t tag = get_u32(buf + off), l = get_u32(buf + off + 4), crc = get_u32(buf + off + 8);
        off += 12;
        if (l > len - off) return -2;
        int ok = brain3_crc32(buf + off, l) == crc;
        Brain3Sec *sec = tag == TAG_SNN ? &idx->snn : tag == TAG_ONTO ? &idx->onto :
                         tag == TAG_IMPR ? &idx->impr : tag == TAG_CRYS ? &idx->crys : NULL;
        if (sec) { sec->ptr = buf + off; sec->len = l; sec->status = ok ? 1 : 2; }
        else idx->n_unknown++;
        off += l;
    }
    if (off != len) return -2;
    return idx->snn.status == 1 ? 0 : -3;
}

size_t brain3_wrap_v2(uint8_t *buf, size_t cap, const uint8_t *v2, size_t v2len)
{
    size_t off = brain3_begin(buf, cap);
    if (!off || v2len > 0xFFFFFFFFu) return 0;
    off = brain3_put(buf, cap, off, TAG_SNN, v2, (uint32_t)v2len);
    if (!off) return 0;
    brain3_finish(buf, off, 1);
    return off;
}

#include "sdm.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

int main(void) {
    printf("[SDM_TEST] Iniciando testes de Sparse Distributed Memory...\n");

    /* 1. Testes de operações hiperdimensionais */
    uint32_t s1 = 12345, s2 = 67890;
    Hypervector a = hv_random(&s1);
    Hypervector b = hv_random(&s2);

    /* Similaridade consigo mesmo é 1.0 */
    assert(fabsf(hv_similarity(a, a) - 1.0f) < 0.001f);

    /* Vetores aleatórios são quase ortogonais (similaridade em torno de 0.0) */
    float sim_ab = hv_similarity(a, b);
    assert(fabsf(sim_ab) < 0.15f);
    (void)sim_ab;

    /* Auto-inverso do bind (XOR) */
    Hypervector bound = hv_bind(a, b);
    Hypervector unbind = hv_bind(bound, b);
    assert(hv_hamming_dist(unbind, a) == 0);
    (void)unbind;

    /* 2. Teste da memória associativa de Kanerva */
    static SparseDistributedMemory sdm;
    sdm_init(&sdm, 42);

    uint32_t s_addr = 0xAA55AA55;
    uint32_t s_data = 0x12345678;
    Hypervector addr1 = hv_random(&s_addr);
    Hypervector data1 = hv_random(&s_data);

    /* Grava dados */
    sdm_store(&sdm, addr1, data1);
    sdm_store(&sdm, addr1, data1); /* Reforço */

    /* Recordação com o endereço exato */
    Hypervector recalled;
    bool ok = sdm_recall(&sdm, addr1, &recalled);
    assert(ok == true);
    (void)ok;
    float sim_exact = hv_similarity(recalled, data1);
    assert(sim_exact > 0.85f);
    printf("  -> Similaridade com endereço exato: %.2f%%\n", sim_exact * 100.0f);

    /* Recordação com endereço corrompido com ruído (pista parcial: 15% de bits trocados) */
    Hypervector noisy_addr = addr1;
    for (int i = 0; i < 4; ++i) {
        noisy_addr.words[i] ^= 0xFFFFFFFFFFFFFFFFULL; /* Inverte 256 bits de 2048 (~12.5% de ruído) */
    }
    Hypervector recalled_noisy;
    ok = sdm_recall(&sdm, noisy_addr, &recalled_noisy);
    assert(ok == true);
    float sim_noisy = hv_similarity(recalled_noisy, data1);
    assert(sim_noisy > 0.70f);
    printf("  -> Similaridade com endereço ruidoso (pista parcial): %.2f%%\n", sim_noisy * 100.0f);

    /* A API tipada distingue memória vazia de uma associação válida ao vetor zero. */
    SparseDistributedMemory typed_memory;
    sdm_init(&typed_memory, 77);
    SdmAssociation association;
    assert(sdm_association_recall(&typed_memory, addr1, &association));
    assert(!association.valid);
    sdm_association_store(&typed_memory, addr1, hv_zero());
    assert(sdm_association_recall(&typed_memory, addr1, &association));
    assert(association.valid);
    assert(hv_hamming_dist(association.value, hv_zero()) == 0);

    SdmEpisodeIndex episode_index;
    sdm_episode_index_init(&episode_index);
    SdmEpisodeRecall episode_recall;
    assert(sdm_episode_index_recall(&episode_index, addr1, &episode_recall));
    assert(!episode_recall.valid);
    assert(sdm_episode_index_store(&episode_index, addr1, hv_zero()));
    assert(sdm_episode_index_recall(&episode_index, addr1, &episode_recall));
    assert(episode_recall.valid && episode_recall.similarity == 1.0f);
    assert(hv_hamming_dist(episode_recall.value, hv_zero()) == 0);
    Hypervector changed_outcome = hv_random(&s_data);
    assert(sdm_episode_index_store(&episode_index, addr1, changed_outcome));
    assert(sdm_episode_index_recall(&episode_index, addr1, &episode_recall));
    assert(hv_hamming_dist(episode_recall.value, changed_outcome) == 0);
    sdm_episode_index_consolidate(&episode_index);
    assert(episode_index.count == 1); /* associação repetida sobrevive ao sono */
    assert(sdm_episode_index_recall(&episode_index, addr1, &episode_recall));
    assert(episode_recall.valid && hv_hamming_dist(episode_recall.value, changed_outcome) == 0);
    for (unsigned i = 0; i < SDM_EPISODE_CAPACITY - 1u; ++i) {
        Hypervector other_key = hv_random(&s_addr);
        Hypervector other_value = hv_random(&s_data);
        (void)sdm_episode_index_store(&episode_index, other_key, other_value);
    }
    assert(episode_index.count <= SDM_EPISODE_CAPACITY);
    sdm_episode_index_consolidate(&episode_index);
    Hypervector overflow_key = hv_random(&s_addr);
    assert(sdm_episode_index_store(&episode_index, overflow_key, changed_outcome));
    assert(episode_index.count <= SDM_EPISODE_CAPACITY);
    static uint8_t episode_buf[20 + sizeof(episode_index.next_slot) + sizeof(episode_index.count) +
                               sizeof(episode_index.episodes)];
    size_t episode_size = sdm_episode_index_serialize(&episode_index, episode_buf,
                                                       sizeof(episode_buf));
    assert(episode_size == sizeof(episode_buf));
    SdmEpisodeIndex episode_loaded;
    sdm_episode_index_init(&episode_loaded);
    assert(sdm_episode_index_deserialize(&episode_loaded, episode_buf, episode_size));
    assert(sdm_episode_index_recall(&episode_loaded, addr1, &episode_recall));
    assert(episode_recall.valid && hv_hamming_dist(episode_recall.value, changed_outcome) == 0);
    episode_buf[episode_size / 2] ^= 1;
    assert(!sdm_episode_index_deserialize(&episode_loaded, episode_buf, episode_size));

    /* 3. Teste de codificação de episódios */
    EpisodeDescriptor ep1 = {
        .window_name = "code - main.c",
        .hour = 14,
        .valence_level = 1,
        .stress_level = 0,
        .event_type = 4 /* sucesso de build */
    };
    Hypervector ep_vec1 = sdm_encode_episode(&ep1);
    Hypervector ep_vec2 = sdm_encode_episode(&ep1);
    assert(hv_hamming_dist(ep_vec1, ep_vec2) == 0); /* Determinismo */
    (void)ep_vec1;
    (void)ep_vec2;

    EpisodeDescriptor ep_diff = ep1;
    ep_diff.hour = 23; /* Outro horário */
    Hypervector ep_vec_diff = sdm_encode_episode(&ep_diff);
    assert(hv_hamming_dist(ep_vec1, ep_vec_diff) > 200);
    (void)ep_vec_diff;

    /* 4. Teste de serialização / desserialização */
    static uint8_t buf[sizeof(sdm.address) + sizeof(sdm.counters) + 128];
    size_t sz = sdm_serialize(&sdm, buf, sizeof(buf));
    assert(sz > 0);

    static SparseDistributedMemory sdm2;
    sdm_init(&sdm2, 99); /* SDM2 precisa restaurar também os endereços. */
    bool des_ok = sdm_deserialize(&sdm2, buf, sz);
    assert(des_ok == true);
    (void)des_ok;

    Hypervector recalled_from_loaded;
    ok = sdm_recall(&sdm2, addr1, &recalled_from_loaded);
    assert(ok == true);
    assert(hv_similarity(recalled_from_loaded, data1) > 0.85f);

    /* Conteúdo truncado, extra, checksum inválido e schema errado são recusados sem mutação. */
    SparseDistributedMemory before_bad_load = sdm2;
    assert(!sdm_deserialize(&sdm2, buf, sz - 1));
    assert(!sdm_deserialize(&sdm2, buf, sz + 1));
    buf[sz / 2] ^= 1;
    assert(!sdm_deserialize(&sdm2, buf, sz));
    buf[sz / 2] ^= 1;
    assert(memcmp(&sdm2, &before_bad_load, sizeof(sdm2)) == 0);

    /* SDM1 é aceito apenas como legado; endereços determinísticos vêm do init do chamador. */
    static uint8_t legacy_buf[sizeof(uint32_t) + sizeof(sdm.counters)];
    uint32_t legacy_magic = 0x53444D31u;
    memcpy(legacy_buf, &legacy_magic, sizeof(legacy_magic));
    memcpy(legacy_buf + sizeof(legacy_magic), sdm.counters, sizeof(sdm.counters));
    SparseDistributedMemory legacy;
    sdm_init(&legacy, 42);
    bool was_legacy = false;
    assert(sdm_deserialize_compatible(&legacy, legacy_buf, sizeof(legacy_buf), &was_legacy));
    assert(was_legacy);
    assert(memcmp(legacy.address, sdm.address, sizeof(sdm.address)) == 0);

    /* 5. Teste de consolidação no sono */
    sdm_consolidate_sleep(&sdm2);
    ok = sdm_recall(&sdm2, addr1, &recalled_from_loaded);
    assert(ok == true);
    assert(hv_similarity(recalled_from_loaded, data1) > 0.80f);

    printf("[SDM_TEST] Todos os testes passaram com sucesso absoluto!\n");
    return 0;
}

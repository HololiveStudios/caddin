# Carmin — plano de eventos e autonomia desktop

**Origem:** plano enviado pelo usuário em 2026-11-22. As propostas deste arquivo são plano de trabalho, não evidência de recursos já validados.
**Estado:** Fase 1 implementada e comparada em um host; validação numérica extensa e fases seguintes pendentes.

## 1. Atualização neural orientada a eventos

- [x] Criar estatísticas de passos: custo, updates efetivos, amostras sensoriais, sinapses percorridas e spikes.
- [x] Substituir a varredura associativa/motora completa por integração de alvos ativados, avançando células silenciosas de forma lazy.
- [x] Ignorar canais sensoriais sem estímulo no passo.
- [x] Validar equivalência da dinâmica com o kernel denso em rede pequena e escalas 10k/100k/1M, com seed fixa e séries de 30–2.000 frames. Hash completo após materialização lazy coincide a precisão quantizada de `1e-9` nos cenários comparados.
- [x] Comparar a distribuição da amostragem geométrica com Bernoulli por unidade em 30 seeds de 100k: diferença média +0,38%, IC95% [-9,07; 38,87] spikes por seed, incluindo zero.
- [x] Executar benchmark dense/event-driven com mesmos inputs, seed, compilador e configuração, isolando o tempo de `brain_step` do custo de criação.
- [x] Medir RSS de processo, updates, amostras propostas, spikes, sinapses e latência em repouso/baixa/média/alta atividade num host.
- [x] Corrigir gargalos encontrados na criação CSR: as regras por pool são resolvidas uma vez, em vez de buscas repetidas por nome por linha.
- [x] Publicar método e resultados observados em [`BASELINE.md`](BASELINE.md), mantendo limites e dados brutos.
- [ ] Repetir a curva em uma segunda arquitetura/hardware e reduzir o pior caso alto antes de alegar tempo real geral.

**Resultado desta rodada:** a curva completa 2k–1M e três intensidades está em [`CURVA_ESCALA.md`](CURVA_ESCALA.md). No milhão, incluindo estímulo e STDP, o ciclo médio ficou em 4,70 ms por eventos contra 51,13 ms denso (10,9×). Na carga alta, ficou em 26,37 ms contra 66,17 ms (2,5×), ultrapassando o orçamento de 16 ms. RSS observado variou entre ~520 e 540 MiB; a inicialização levou 6,6–8,5 s. Os números são deste host e não incluem sensores/renderização reais.

## 2. Grupos sensoriais, associativos e motores

- [x] Auditar os 64 grupos sensoriais, produtores reais e limitações atuais.
- [x] Auditar os 32 associativos, projeções declaradas, fan-out e consumidores no app.
- [x] Auditar os 24 motores e consumidores atuais do mascote.
- [x] Especificar proposta completa de 64/32/24 grupos desktop com origem-alvo e escala preservada em [`MAPA_GRUPOS_DESKTOP.md`](MAPA_GRUPOS_DESKTOP.md).
- [ ] Fixar conectividade, migração de estado e métricas de cada semântica nova antes de ativá-la.
- [ ] Manter os IDs existentes compatíveis até existir plano de migração e teste.
- [ ] Validar novos e antigos grupos com seeds e cenários reproduzíveis.

## 3. Actuator Guard e privacidade

- [x] Definir catálogo de ações e esquema tipado de proposta em [`ACTUATOR_GUARD.md`](ACTUATOR_GUARD.md).
- [x] Implementar guard determinístico separado da rede, com política por ação, confirmação pontual, expiração e cancelamento explícito.
- [x] Criar resultados terminais cancelado/falhou para alimentar avaliação sem payload.
- [x] Criar adaptador genérico assíncrono limitado a 16 solicitações, payload 256 B, deadline, cancelamento e retorno estruturado.
- [x] Manter ações externas bloqueadas por padrão; auditar somente metadados e decisão, sem payload sensível.
- [ ] Revisar permissões de sensores, retenção, minimização, opt-out e indicação visível de captura.
- [x] Exercitar recusa, expiração, privacidade, alvo desconhecido, confirmação e rate limit em `ActuatorGuardTest`.
- [ ] Implementar UI de confirmação e adaptadores de executor que nunca possam contornar o guard.
- [ ] Ligar o adaptador a um worker/serviço real, persistir resultado mínimo e exercitar latência do núcleo durante timeout/quedas; nenhum LLM ou executor real está conectado.

## 4. Modos de aprendizado

- [x] **Shadow:** default da biblioteca; só observa e nega ações.
- [x] **Assisted:** biblioteca exige confirmação pontual antes de aplicar allowlisted action.
- [x] **Autonomous:** biblioteca só permite adaptação interna/status reversível previamente allowlisted.
- [ ] Definir migração de estado, logs mínimos, rollback e critérios de promoção entre modos.
- [ ] Integrar modos a configuração/UI e ao loop de aprendizado sem mudar comportamento legado fora do perfil escolhido.

## Critérios de conclusão deste plano

As fases são independentes de seus rótulos: configuração de neurônios não prova utilidade, tempo real, segurança de ações ou qualidade de aprendizado. Cada item exige evidência reproduzível, documentação e resultados compatíveis com o hardware/plataforma declarados.

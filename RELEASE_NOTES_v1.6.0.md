# Carmin v1.6.0

Esta versão mantém 1.048.576 unidades distribuídas nos 120 grupos e inclui medição do kernel event-driven e novos contratos para memória, eventos, telemetria e propostas de ação.

## Mudanças

- Atualização LIF event-driven com avanço lazy de estados silenciosos e estatísticas de custo; referência densa e curvas de 2k, 10k, 100k e 1M documentadas.
- `CarminEvent` v1 ligado ao loop síncrono com sequência, timestamp, origem, flags de disponibilidade/privacidade e valores escalares; não carrega conteúdo bruto de tela, texto ou áudio.
- Persistência SDM2 versão/schema 2 com parâmetros, endereços, pesos de localização, total de gravações e CRC32. SDM1 legado não tinha endereços; o app o reconhece e reinicializa a memória em vez de mapear contadores incorretamente.
- Índice episódico experimental, limitado a 256 registros, com schema/CRC e round-trip validado; harness de associação com relógio virtual e protocolo reproduzível. O app carrega e salva esse índice, mas ainda não insere episódios reais do uso diário.
- `CarminActuatorGuard` v1 em biblioteca isolada, default Shadow, confirmação Assisted pontual, restrição Autonomous, allowlist, expiração, rate limit e auditoria somente de metadados.
- `CarminAsyncQueue` limitada a 16 requests e 256 bytes por payload, com deadline, cancelamento e resultado estruturado.
- `CarminTelemetrySnapshot` local read-only com 120 taxas e fingerprint de população/grupos.
- README, plano M0–M6, inventário e documentação de arquitetura atualizados.

## Validação

- Build Release Linux x86_64 e cross-build Release Windows x86_64 gerados.
- Suíte Linux Release completa: 17/17 passou. Os testes Windows foram compilados, mas não executados no host Linux sem Wine/Windows.
- Benchmark neural em Intel Core i7-7700T: 4,70 ms ciclo médio e 26,37 ms carga alta no perfil 1M; carga alta excede 16 ms. RSS de processo observado: ~520–540 MiB.
- Harness SDM de seed 42: 100% chave exata; 97,85% com 6,25% de ruído; 100% após consolidação/replay neste cenário estreito.
- Simulação temporal 720 frames, seed 42: 695/720 (96,53%), 24 stale, 1 vazia, hash `67aa7c1001b4ae12`.

## Limites explícitos

- M0–M5 continuam parciais; M6 depende de outra arquitetura/hardware, métricas de aplicação e utilidade funcional.
- Nenhum executor de mouse/teclado/arquivos/shell, UI para modos/consentimento, worker LLM, replay de eventos reais ou publicação do snapshot está conectado ao runtime.
- A topologia usa fan-out máximo 64 e 52.166.656 sinapses observadas. Os grupos desktop novos permanecem propostos; IDs existentes não foram migrados.
- Benchmark de SDM/simulação é sintético e ainda não foi repetido em campanhas multiseed.

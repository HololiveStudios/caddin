# Carmin — Evolução arquitetural

**Estado:** alvo de 1.048.576 unidades configurado; utilidade/tempo real ainda precisam de validação
**Atualizado:** 2026-11-22
**Versão documental:** 0.3

Este é o ponto de entrada para futuras sessões de evolução. Leia este arquivo antes de retomar e consulte apenas os documentos relacionados à etapa atual.

## Identidade e escopo

- **Nome do produto em evolução:** Carmin.
- **Nome atual do repositório e de compatibilidade:** Caddin. O nome do diretório, binários, configuração `.caddinrc` e arquivos persistidos ainda não foram migrados.
- **Objetivo arquitetural:** manter 1.048.576 unidades neuronais indexáveis com estado individual e conectividade esparsa; validar comportamento, custo e desempenho com medições reais.
- **Escopo da atualização 0.3:** configurar a escala 512 nos 120 grupos, substituir a matriz de construção por CSR com fan-out máximo de 64, mover vetores de estado para heap, adaptar o caminho de migração. Suíte de testes não executada.

## Estado observado no repositório

- Linguagem/build: C11, CMake e Make; caminhos Linux/X11, Windows, Android e headless estão declarados no build.
- O caminho ativo de comportamento em `src/main.c` usa `brain_create/brain_step` de `neuro.c`; o SNN de `src/core/snn.*` continua não sendo o decisor do app.
- A rede ativa configura **1.048.576 unidades**, preservando os 120 IDs de grupo: 262.144 sensoriais, 589.824 associativas e 196.608 motoras. Cada população-base usa escala 512.
- A matriz densa temporária foi removida da construção e migração. As regras são amostradas em CSR com fan-out máximo de 64 por unidade, mantendo o custo de armazenamento linear. Esta restrição altera a densidade efetiva de projeções em escala alta.
- Foram nomeados dois slots anteriormente genéricos: `CONTEXT_BINDING` e `UNCERTAINTY_MONITOR`. O inventário registra parâmetros, faixa de IDs e regras de projeção.
- A configuração agora valida grupos únicos/não vazios e referências, pesos e densidades das projeções antes de criar o cérebro. Corrigi as referências inválidas `FRUSTRATED` para `FRUSTRATION`.
- A persistência passa a gravar pesos CSR com checksum e inclui migração dos formatos densos de 2.048 unidades; a migração duplica trilhas por réplica alinhada e normaliza pesos pela escala.
- A configuração possui 201 regras de projeção: 120 grupos únicos, nenhuma referência ausente e soma de 1.048.576 unidades.
- Os canais de sistema para CPU e temperatura agora alimentam a SNN; `PAUSE`, backspace, delete, tab, símbolo e pontuação ganharam codificação de evento/classe. Ruído de microfone disponível alimenta `NOISE`. Seis canais continuam sem produtor confiável e estão identificados no inventário.
- Há módulos para arena, neuromodulação, CPG, predição, persistência, ontogenia, SDM, regras de configuração, sensores, corpo/animação, comportamento, linguagem e telemetria.
- A SDM atual declara 128 localizações e hipervetores de 2.048 bits em `src/core/sdm.h`, bem abaixo das 65.536 localizações propostas no blueprint.
- O build inclui uma API neural legada em arquivos de raiz (`neuro.c/.h`) e o núcleo `src/core/snn.*`. O caminho de `neuro.c` está ativo; `src/core/snn.*` ainda não está ligado ao decisor do app.
- Não encontrei no CMake atual os componentes de agente/LLM, o barramento cognitivo do blueprint nem o renderizador holográfico especificado nele.
- `git status` já continha diretórios de build Android não rastreados antes da edição documental; eles foram deixados intactos.

## Arquitetura-alvo em avaliação

O desenho preferido mantém o runtime principal em C11 e separa: entrada de eventos e filtros de privacidade; codificadores; populações neurais com fidelidades diferentes; memória associativa/episódica; estado homeostático; política de ações; e adaptadores assíncronos para modelos externos. O núcleo de tempo real não deve esperar por disco, rede ou LLM. A política, fora da rede e do LLM, permanece autoridade para permitir ações.

O contrato final ainda não está congelado. Em particular, formatos numéricos, mapa fixo de 64 grupos, ponto de 1 ms para toda população, tamanhos de arena, densidade de sinapses e os limites de latência do blueprint precisam de protótipo, medição e revisão. Consulte [Arquitetura e decisões](docs/carmin/ARQUITETURA_ALVO.md).

## Documentos de planejamento

- [Arquitetura-alvo e decisões pendentes](docs/carmin/ARQUITETURA_ALVO.md)
- [Plano de implementação M0–M6](docs/carmin/PLANO_IMPLEMENTACAO.md)
- [Inventário completo dos 120 grupos e status das entradas](docs/carmin/INVENTARIO_GRUPOS.md)
- [Referências técnicas verificadas](docs/carmin/REFERENCIAS.md)

## Componentes e validação

**Implementado:** escala 512 configurada nos grupos existentes, CSR direto com fan-out máximo 64, vetores de estado em heap, validação das definições de grupo/projeção, introspecção de grupo/sinapse, dois grupos funcionais nomeados, serialização CSR e migração compatível por projeção disponível.

**Pendente:** atualização esparsa/lazy do LIF e sensores, benchmark de RSS/CPU/latência, validação comportamental e de persistência, resolver canais sem produtor, revisar qualidade da topologia limitada por grau e dimensionar grupos funcionais novos com hipótese/entrada/saída observáveis. A simulação ainda percorre densamente 262.144 unidades sensoriais e 786.432 unidades associativas/motoras por milissegundo simulado, além de zerar o vetor de drive; tempo real não foi comprovado.

**Verificação nesta atualização:** compilação do alvo `caddin` concluída; revisão estática e `git diff --check` realizados. Ainda não foi medido RSS/CPU nem executada a suíte de testes. A rede configura um milhão, mas o passo atual ainda é denso e não tem desempenho de tempo real comprovado.

## Decisões documentadas

1. Usar Carmin como identidade dos novos planos, sem fingir que o código e os artefatos legados já foram renomeados.
2. Preservar e medir os módulos atuais antes de substituição.
3. Não incorporar como verdades os números e contratos fechados do blueprint fornecido; tratá-los como hipóteses rastreáveis até validação.
4. Nenhuma dependência nova será adicionada até que um protótipo demonstre vantagem que justifique o custo de integração e distribuição.

## Próxima tarefa exata

Implementar atividade lazy/event-driven por pool, mantendo a dinâmica e taxa de disparo verificáveis; depois medir RSS, bytes de conectoma, CPU, latência e comportamento em cenários reproduzíveis. O tamanho configurado já é 1.048.576; não divulgar desempenho sem esses dados.

## Histórico resumido

- **0.1 — 2026-11-22:** leitura dos documentos e planejamento inicial Carmin.
- **0.3 — 2026-11-22:** rede configurada para 1.048.576 unidades; construtor CSR com grau máximo; vetores no heap; migração adaptada; compilação concluída, desempenho ainda pendente.

# Carmin v1.5.0

Carmin passa a configurar a rede ativa para 1.048.576 unidades distribuídas em 120 grupos, preservando o mapeamento atual dos grupos e dos canais.

## Mudanças

- 64 grupos sensoriais × 4.096 unidades = 262.144.
- 32 grupos associativos × 18.432 unidades = 589.824.
- 24 grupos motores × 8.192 unidades = 196.608.
- Conectoma construído diretamente em CSR sem matriz temporária N×NL.
- Fan-out limitado a 64 sinapses por neurônio; a densidade efetiva é amostrada por projeção e pode ficar abaixo das densidades nominais.
- Vetores de estado alocados no heap e migração de checkpoints legados para arestas correspondentes na topologia ativa.
- README, inventário de grupos e planos de arquitetura atualizados.
- Pacotes Release Linux x86_64 e Windows x86_64.

## Validação e limites conhecidos

- Builds Release Linux e Windows concluídos.
- A suíte de testes não foi executada para esta release.
- 1.048.576 é o total de unidades configuradas com estado individual; não representa benchmark de tempo real.
- O passo ainda varre densamente as populações por milissegundo simulado. RSS, CPU, latência e utilidade comportamental na escala ainda não foram medidos.
- A reorganização de grupos para controle geral de desktop, atuadores, operações de arquivos/shell ou integração de LLM permanece planejada, não implementada.

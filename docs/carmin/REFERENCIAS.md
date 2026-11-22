# Carmin — Referências técnicas

**Verificadas em:** 2026-11-22
As referências informam decisões e ferramentas de pesquisa; sua inclusão não significa adoção como dependência.

## Simulação SNN e ferramentas neuroinspiradas

- [NEST Simulator — projeto oficial](https://github.com/nest/nest-simulator): simulador de redes de neurônios pontuais, com foco em dinâmica, tamanho e estrutura. Serve de referência para modelos e metodologia de simulação. O repositório declara GPL v2 ou posterior; qualquer incorporação exige revisão da licença e forma de distribuição.
- [NEST — documentação de execução de simulações](https://nest-simulator.readthedocs.io/en/stable/nest_behavior/running_simulations.html): recursos de execução e medição; usar para comparar desenho de experimentos, não como promessa de desempenho do Carmin.
- [Brian 2 — métodos computacionais e eficiência](https://brian2.readthedocs.io/en/2.0/user/computation.html): documenta geração standalone C++ e a aplicabilidade variável dessa modalidade. Útil para prototipagem e validação independente de regras, sem dependência de runtime.
- [Brian 2: an intuitive and efficient neural simulator (eLife)](https://doi.org/10.7554/eLife.47314): publicação revisada por pares sobre os modos de execução e geração de código de Brian 2.
- [Lava — documentação oficial](https://lava-nc.org/): framework aberto para aplicações neuroinspiradas e mapeamento para hardware neuromórfico. Considerar apenas com caso de uso/hardware definido.

## Agentes e simulação multiagente

- [PettingZoo — documentação oficial](https://pettingzoo.farama.org/): API para ambientes multiagentes; possível harness externo para coordenação e competição entre políticas.
- [Ray Core — documentação oficial](https://docs.ray.io/en/latest/ray-core/walkthrough.html): tasks, actors e objetos para computação distribuída em Python. Pode servir à execução de campanhas experimentais; não é requisito para o runtime desktop C.

## LLM local

- [llama.cpp server README oficial](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md): descreve servidor e endpoints compatíveis com OpenAI. A integração do Carmin deve continuar assíncrona, limitada por timeout e submetida a validação de saída e política local.

## Dados de referência fornecidos pelo usuário

- `carmin_1m_neuron_architecture_implementation_blueprint.md` (arquivo local fornecido): especifica hipótese fechada de 1.048.576 LIF, mapa de regiões, arena, memória, agentes, LLM, simulação e renderização.
- `Texto colado.txt` em anexos (documento de pesquisa organizado): distingue fatos com citação de decisões/estimativas de engenharia, propõe fidelidades F0–F2, atualização esparsa, separação de sinais de ensino e estados lentos, política de ações e fases dependentes.

As duas fontes são material de entrada. O código local é a evidência para o estado atual; estimativas devem ser recalculadas e testadas antes de virarem critérios de aceite.

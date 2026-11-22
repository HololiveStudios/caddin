# Simulação headless de associação temporal

`simulate_association` executa 720 frames de relógio virtual, 24 contextos com troca a cada 30 frames, uma reversão de resultado no frame 360 e consolidação a cada 60 frames. A seed é fixa; um hash de frames e quantidade de episódios permite comparar replays. Nenhuma entrada do SO ou texto real é lido. O exemplo mede acertos antes de gravar a associação daquele frame, associações vazias e respostas antigas após a mudança de resultado. Para evitar a mistura de outcomes na métrica supervisionada, a simulação usa um índice episódico limitado separado; a SDM continua como mecanismo distribuído de recall.

Reprodução:

```sh
cmake -S . -B /tmp/carmin-build -DCMAKE_BUILD_TYPE=Release -DCADDIN_HEADLESS=ON
cmake --build /tmp/carmin-build --target simulate_association -j2
/tmp/carmin-build/simulate_association
```

Este é um harness inicial, não prova ainda generalização ou retenção apropriada. O índice exato fornece referência para associação episódica e troca de resultado, mas não generaliza chaves vizinhas; a SDM entrega generalização distribuída com risco de mistura. O índice limita a 256 slots; em saturação, substitui o registro com menor número de reforços (política FIFO aproximada), mantendo a capacidade limitada. Para avaliar em estatística, repetir dezenas de seeds e variar número de tarefas, ruído, duração, frequência de reforço e tempo de esquecimento, comparando baseline fixo e SDM no mesmo stream.

Coleta inicial na seed 42: 695 acertos de 720 (96,53%), 24 respostas stale antes da revisão/reaprendizado, 1 consulta vazia e hash `67aa7c1001b4ae12`. Foi um único replay, sem comparação multiseed; não é critério suficiente para M3 concluído.

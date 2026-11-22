# Contrato v1 de eventos cognitivos Carmin

## Campos

`CarminEvent` em [`src/core/cognitive_event.h`](../../src/core/cognitive_event.h) contém versão e tamanho do registro, sequência monotônica, timestamp monotônico em nanossegundos de simulação, canal `0..63`, origem, flags e valor escalar `float` limitado a `[-1,1]`. O envelope não permite payload de texto, bytes de áudio, pixels nem caminho de arquivo.

Origem é um enum tipado: encoder do app, monitor do SO, microfone, metadados de janela, estado interno, evento derivado ou desconhecido. Um evento desconhecido não pode alegar disponibilidade. Flags distinguem disponível/indisponível, sensível, redigido, bloqueado por privacidade e sintético.

## Validação e consumidor

- Versão, tamanho, canal, origem, valor finito e idade são verificados antes de entrar no vetor SNN.
- Evento `SENSITIVE` só é aceito se explicitamente `REDACTED` ou `PRIVACY_BLOCKED`; bloqueado/indisponível chega ao encoder como zero.
- Eventos repetidos no mesmo canal usam a sequência mais nova. Valores negativos são reduzidos a zero no adaptador de intensidade, preservando a regra do `brain_stimulate` anterior.
- O produtor cria um lote stack-local com 64 valores por frame e o consumidor lê imediatamente no mesmo thread. Assim, M1 não cria fila SPSC/MPSC: não existe concorrência de produtores justificada no fluxo observado.
- `src/main.c` marca CPU/temperatura como monitor do sistema, ruído alto como microfone quando disponível, entradas de sessão como encoder/estado interno e os seis canais sem produtor como indisponíveis.

## Privacidade, replay e persistência

O lote atual carrega apenas escalares agregados, não conteúdo bruto de OCR, texto, áudio ou arquivos. A política da origem precisa bloquear contexto sensível antes que no futuro seja convertido em uma referência/feature; marcar um escalar como redigido não substitui apagar o dado bruto do produtor.

O benchmark fixa seed, valores de entrada e sequência de frames e oferece replay sintético determinístico. Ainda não existe gravação/replay de eventos reais do SO. Persistência neural guarda CSR/pesos com formato e topologia hash; o vetor transitório de eventos não é persistido. A aplicação lê/escreve snapshots no thread principal, portanto não há corrida hoje, mas snapshots concorrentes futuros precisarão de cópia imutável ou troca de ponteiro validada.

`CarminTelemetrySnapshot` v1 oferece captura local read-only de contadores, taxa por 120 regiões e hash dos limites/mapa de população para consumo futuro. No estado atual a estrutura é construída e descartada localmente em 1Hz; ainda não é exportada ou persistida. `CarminAsyncQueue` é uma fila limitada síncrona por contrato, ainda sem produtor/worker real no app. Seu payload é limitado a 256 bytes; para requests a LLM deve conter identificador estruturado/minimizado, nunca texto de tela arbitrário.

O teste `CognitiveEventTest` cobre validação de versão, frescor, canal, valor, ordem de sequência, origem e flags de privacidade. Extensões binárias precisam incrementar a versão e migrar explicitamente; não serializar o layout C em disco como ABI estável.

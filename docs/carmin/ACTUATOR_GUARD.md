# Actuator Guard e modos de aprendizado

**Estado:** núcleo determinístico implementado; nenhum executor de desktop está conectado. `src/main.c` ainda não carrega perfil de modo nem envia propostas ao guard. Os comportamentos e a voz já existentes do mascote continuam no caminho legado.

## Contrato de proposta

`CarminActionProposal` v1 carrega ID, token de contexto, token de alvo opaco, criação/expiração, tipo, confiança e flags. O registro não carrega texto, caminho de arquivo, comando, teclas ou conteúdo de tela. Um executor futuro recebe os dados de payload de um adaptador validado somente depois de o guard autorizar a mesma proposta/alvo.

O guard valida versão/tamanho, tempo de expiração, alvo conhecido, flags privadas/sensíveis/críticas, allowlist por tipo, confirmação e limite de ações por minuto. Após execução, um futuro adaptador pode registrar resultado válido, cancelado ou falhou para alimentar avaliação. Decisões e auditoria guardam apenas ID, tipo, timestamp, flags e resultado. O guard **autoriza ou recusa**; não executa mouse, teclado, arquivos, shell, voz ou LLM.

## Modos implementados na biblioteca

| Modo | Regra |
|---|---|
| Shadow | Default de `carmin_guard_init`; apenas `OBSERVE` gera decisão registrada. Outras propostas retornam `SHADOW_ONLY`; allowlist começa vazia. |
| Assisted | Ações precisam de allowlist e confirmação pontual ligada ao ID, contexto e alvo. Confirmação expira em até 5 s e só vale uma vez. |
| Autonomous | Mesmo com allowlist, só aceita adaptação interna/indicador reversível. Clique, digitação, arquivos, shell, LLM e voz externa são recusados. |

Todos os modos bloqueiam flag privada/sensível/crítica. A política não confia em saída de SNN/LLM como autorização. A auditoria é um buffer volátil de 64 decisões sem payload.

## Estado de integração e pendências

- A biblioteca e `ActuatorGuardTest` exercitam allowlist vazia, Shadow, privacidade, alvo desconhecido, expiração, confirmação errada/ausente, uso único, limite de taxa e restrição autônoma.
- Resultados terminais aceitos no registro são apenas `CANCELLED` e `FAILED`; a biblioteca não permite que um executor forje resultado `ALLOWED`.
- Não existe diálogo/UI para confirmar propostas, armazenamento de preferências nem executor real; Assisted/Autonomous ainda não podem ser ativados pelo app.
- A blacklist existente protege alguns leitores de contexto; ela ainda não é uma política central de permissões/retenção/opt-out de todos os sensores. Não enviar conteúdo sensível até essa revisão terminar.
- Ligar novo output motor ao guard só após identificar alvo por token de contexto, mapear blacklist global e testar todas as recusas antes do executor.

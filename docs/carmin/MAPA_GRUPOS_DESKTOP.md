# Carmin — mapa-alvo dos 120 grupos desktop

**Estado:** proposta de arquitetura; nenhum dos nomes desta tabela substitui grupos ativos ainda. O mapa parte das categorias do plano enviado e nomeia funções individuais para que entradas/saídas possam ser verificadas.

## Distribuição preservada

| IDs | Tipo | Número | Tamanho por grupo | Total |
|---|---|---:|---:|---:|
| 0–63 | Sensoriais | 64 | 4.096 | 262.144 |
| 64–95 | Associativos | 32 | 18.432 | 589.824 |
| 96–119 | Motores/propostas | 24 | 8.192 | 196.608 |

Total configurado permanece 1.048.576. Reaproveitar IDs não basta para migrar semântica/pesos: a persistência precisa versionar o mapa de grupos, a topologia e a codificação antes de substituir os perfis ativos.

## 64 grupos sensoriais propostos

### IDs 0–19 — visão, tela e leitura

| ID | Nome-alvo | Evento observável planejado |
|---:|---|---|
| 0 | `SCREEN_WINDOW` | classe/título da janela ativa após filtro de privacidade |
| 1 | `SCREEN_TEXT_NEW` | texto novo permitido, sem segredos |
| 2 | `SCREEN_TEXT_CONTEXT` | contexto textual agregado da janela |
| 3 | `SCREEN_LAYOUT` | mudança de layout/elementos visuais |
| 4 | `SCREEN_ALERT` | alerta visual reconhecido |
| 5 | `SCREEN_BUILD_ERROR` | erro de build/terminal autorizado |
| 6 | `SCREEN_BUILD_SUCCESS` | conclusão bem-sucedida autorizada |
| 7 | `SCREEN_DIALOG` | diálogo ou modal ativo |
| 8 | `SCREEN_APP_CLASS` | categoria de aplicativo, sem guardar conteúdo bruto |
| 9 | `SCREEN_CHANGE` | delta visual agregado |
| 10 | `READING_FOCUS` | indício de leitura/foco sustentado |
| 11 | `SCREEN_SELECTION` | seleção/copiar texto em contexto permitido |
| 12 | `SCREEN_CODE_CONTEXT` | contexto de edição de código, filtrado |
| 13 | `SCREEN_DOC_CONTEXT` | contexto de documento, filtrado |
| 14 | `SCREEN_PRIVACY_BLOCK` | indicação de bloqueio/blacklist |
| 15 | `SCREEN_CAPTURE_STATE` | sensor ativo, pausado ou indisponível |
| 16 | `SCREEN_OCR_CONFIDENCE` | confiança agregada do reconhecimento |
| 17 | `SCREEN_TASK_SWITCH` | troca de janela/tarefa |
| 18 | `SCREEN_NOTIFICATION` | notificação permitida e minimizada |
| 19 | `SCREEN_REDACTION` | volume agregado de texto removido por privacidade |

### IDs 20–31 — áudio e voz

`20 AUDIO_RMS`, `21 AUDIO_VAD`, `22 VOICE_TONE`, `23 VOICE_URGENCY`, `24 AUDIO_SYSTEM`, `25 SPEECH_ACTIVITY`, `26 SPEAKER_MATCH`, `27 AUDIO_MUSIC`, `28 AUDIO_NOISE`, `29 AUDIO_TRANSIENT`, `30 AUDIO_PAUSE`, `31 AUDIO_SOURCE_STATE`.

### IDs 32–47 — teclado, mouse e biometria de interação

`32 KEY_CADENCE`, `33 KEY_HESITATION`, `34 KEY_SHORTCUT`, `35 KEY_TEXT_CLASS`, `36 KEY_BACKSPACE`, `37 KEY_ENTER`, `38 KEY_TAB`, `39 KEY_EDIT_PATTERN`, `40 MOUSE_POSITION`, `41 MOUSE_VELOCITY`, `42 MOUSE_CLICK`, `43 MOUSE_SCROLL`, `44 MOUSE_DRAG`, `45 MOUSE_SELECTION`, `46 MOUSE_IDLE`, `47 INPUT_BURST`.

### IDs 48–63 — sistema, arquivos, processos e rede

`48 CPU_LOAD`, `49 MEMORY_PRESSURE`, `50 SYSTEM_TEMPERATURE`, `51 PROCESS_CHANGE`, `52 ACTIVE_WINDOW_CHANGE`, `53 FILE_CHANGE`, `54 FILE_READ_RESULT`, `55 FILE_WRITE_RESULT`, `56 NOTIFICATION_STATE`, `57 NETWORK_STATE`, `58 BATTERY_STATE`, `59 POWER_STATE`, `60 SYSTEM_UPTIME`, `61 PRIVACY_CONTEXT`, `62 TOOL_RESULT`, `63 SENSOR_AVAILABILITY`.

## 32 grupos associativos propostos

- **64–73 — compreensão multimodal/foco:** `MULTIMODAL_FUSION`, `TASK_CONTEXT`, `CODE_CONTEXT`, `READING_CONTEXT`, `FOCUS_ESTIMATE`, `PREDICTION_ERROR`, `SEMANTIC_MEMORY`, `EPISODIC_LINK`, `GOAL_CONTEXT`, `USER_INTENT`.
- **74–81 — planejamento/rotina:** `TASK_SEQUENCE`, `PLAN_STEP`, `ROUTINE_PATTERN`, `TASK_PRIORITY`, `WORKFLOW_MEMORY`, `INTERRUPT_GATE`, `TOOL_ROUTING`, `RESULT_EVALUATION`.
- **82–89 — estado interno/homeostase/permissão:** `MOTIVATION_STATE`, `ATTENTION_STATE`, `ALERT_STATE`, `CALM_CONFIDENCE`, `FATIGUE_STATE`, `CURIOSITY_STATE`, `TRUST_PERMISSION`, `HOMEOSTATIC_CONTROL`.
- **90–95 — filtro de LLM:** `LLM_NEEDED`, `SEMANTIC_COMPLEXITY`, `UNCERTAINTY`, `CONTEXT_PACK`, `LLM_RESPONSE_VALIDATION`, `LLM_BUDGET_GATE`.

## 24 saídas motoras propostas

Todas são **propostas tipadas**, nunca uma ação executável por si só. O Actuator Guard valida tipo, alvo, permissão, privacidade, taxa e confirmação, e o executor independente recebe apenas propostas aprovadas.

- **96–101 — mouse:** `MOUSE_MOVE_PROPOSAL`, `MOUSE_CLICK_PROPOSAL`, `MOUSE_SCROLL_PROPOSAL`, `MOUSE_DRAG_PROPOSAL`, `MOUSE_SELECT_PROPOSAL`, `MOUSE_HOVER_PROPOSAL`.
- **102–107 — teclado:** `KEY_TYPE_PROPOSAL`, `KEY_AUTOCOMPLETE_PROPOSAL`, `KEY_SHORTCUT_PROPOSAL`, `KEY_TAB_PROPOSAL`, `KEY_ENTER_PROPOSAL`, `KEY_EDIT_PROPOSAL`.
- **108–113 — arquivo/shell:** `FILE_READ_PROPOSAL`, `FILE_WRITE_PROPOSAL`, `FILE_ORGANIZE_PROPOSAL`, `FILE_MOVE_PROPOSAL`, `SHELL_SCRIPT_PROPOSAL`, `APP_SWITCH_PROPOSAL`.
- **114–117 — LLM/tarefas:** `LLM_CONTEXT_PROPOSAL`, `LLM_REQUEST_PROPOSAL`, `TASK_DELEGATE_PROPOSAL`, `LLM_RESULT_ACCEPT_PROPOSAL`.
- **118–119 — voz/presença:** `VOICE_OUTPUT_PROPOSAL`, `STATUS_INDICATOR`.

## Requisitos para ativar o mapa

1. Implementar fonte e condição de indisponibilidade por canal; marcar claramente o que exige OCR, hook global, watcher de arquivos ou serviço externo.
2. Implementar redaction/blacklist antes de codificar, persistir ou encaminhar contexto.
3. Versionar mapa, codificador e topologia. Migrar checkpoints em arquivo temporário, comparar, e só então substituir o anterior atomicamente.
4. Fazer o shadow replay dos grupos-alvo sem alterar saídas; aprovar os resultados antes de conectar a política.
5. Ligar saídas apenas ao Actuator Guard e exercitar recusa, expiração, alvo privado e confirmação ausente.

O status de sensores/produtores e o inventário de grupos atuais estão em [`INVENTARIO_GRUPOS.md`](INVENTARIO_GRUPOS.md). As capacidades inexistentes continuam marcadas como propostas, não como features prontas.

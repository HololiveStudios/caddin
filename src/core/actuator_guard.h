#ifndef CARMIN_ACTUATOR_GUARD_H
#define CARMIN_ACTUATOR_GUARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CARMIN_GUARD_VERSION 1u
#define CARMIN_GUARD_AUDIT_CAPACITY 64u
#define CARMIN_GUARD_RATE_CAPACITY 64u

typedef enum {
    CARMIN_MODE_SHADOW = 0,
    CARMIN_MODE_ASSISTED = 1,
    CARMIN_MODE_AUTONOMOUS = 2
} CarminLearningMode;

typedef enum {
    CARMIN_ACTION_OBSERVE = 0,
    CARMIN_ACTION_INTERNAL_ADAPT,
    CARMIN_ACTION_STATUS_INDICATOR,
    CARMIN_ACTION_CURSOR_MOVE,
    CARMIN_ACTION_CLICK,
    CARMIN_ACTION_SCROLL,
    CARMIN_ACTION_DRAG,
    CARMIN_ACTION_TYPE_TEXT,
    CARMIN_ACTION_FILE_READ,
    CARMIN_ACTION_FILE_WRITE,
    CARMIN_ACTION_FILE_MOVE,
    CARMIN_ACTION_SHELL,
    CARMIN_ACTION_LLM_REQUEST,
    CARMIN_ACTION_VOICE_OUTPUT,
    CARMIN_ACTION_COUNT
} CarminActionKind;

enum {
    CARMIN_PROPOSAL_TARGET_KNOWN = 1u << 0,
    CARMIN_PROPOSAL_REVERSIBLE   = 1u << 1,
    CARMIN_PROPOSAL_SENSITIVE   = 1u << 2,
    CARMIN_PROPOSAL_CRITICAL    = 1u << 3,
    CARMIN_PROPOSAL_PRIVATE      = 1u << 4
};

typedef struct {
    uint16_t version;
    uint16_t size;
    uint64_t id;
    uint64_t context_token;
    uint64_t target_token;
    uint64_t created_ns;
    uint64_t expires_ns;
    uint16_t action;
    uint16_t reserved;
    uint32_t flags;
    float confidence;
} CarminActionProposal;

typedef enum {
    CARMIN_GUARD_ALLOWED = 0,
    CARMIN_GUARD_OBSERVED,
    CARMIN_GUARD_SHADOW_ONLY,
    CARMIN_GUARD_CONFIRMATION_REQUIRED,
    CARMIN_GUARD_DENY_INVALID,
    CARMIN_GUARD_DENY_EXPIRED,
    CARMIN_GUARD_DENY_PRIVACY,
    CARMIN_GUARD_DENY_UNKNOWN_TARGET,
    CARMIN_GUARD_DENY_NOT_ALLOWLISTED,
    CARMIN_GUARD_DENY_MODE,
    CARMIN_GUARD_DENY_RATE_LIMIT,
    CARMIN_GUARD_DENY_CONFIRMATION,
    CARMIN_GUARD_RESULT_CANCELLED,
    CARMIN_GUARD_RESULT_FAILED
} CarminGuardDecision;

typedef struct {
    uint64_t sequence;
    uint64_t timestamp_ns;
    uint64_t proposal_id;
    uint16_t action;
    uint16_t decision;
    uint32_t flags;
} CarminGuardAudit;

typedef struct {
    CarminLearningMode mode;
    uint32_t allow_mask;
    uint16_t max_actions_per_minute;
    uint16_t rate_count;
    uint16_t rate_next;
    uint16_t audit_head;
    uint16_t audit_count;
    uint64_t accepted_at[CARMIN_GUARD_RATE_CAPACITY];
    CarminGuardAudit audit[CARMIN_GUARD_AUDIT_CAPACITY];
    bool pending_valid;
    bool approved_valid;
    uint64_t pending_id;
    uint64_t pending_context;
    uint64_t pending_target;
    uint64_t pending_deadline_ns;
    uint16_t pending_action;
    uint64_t approved_id;
    uint64_t approved_context;
    uint64_t approved_target;
    uint16_t approved_action;
    uint64_t next_audit_sequence;
} CarminActuatorGuard;

/* Default policy is Shadow with no action allowlist or execution authority. */
void carmin_guard_init(CarminActuatorGuard *guard);
bool carmin_guard_configure(CarminActuatorGuard *guard, CarminLearningMode mode,
                            uint32_t allow_mask, uint16_t max_actions_per_minute);
CarminGuardDecision carmin_guard_evaluate(CarminActuatorGuard *guard,
                                          const CarminActionProposal *proposal,
                                          uint64_t now_ns);
bool carmin_guard_confirm(CarminActuatorGuard *guard, uint64_t proposal_id,
                          uint64_t context_token, uint64_t target_token,
                          CarminActionKind action, uint64_t now_ns);
bool carmin_guard_cancel_pending(CarminActuatorGuard *guard, uint64_t proposal_id,
                                uint64_t now_ns);
bool carmin_guard_record_result(CarminActuatorGuard *guard,
                                const CarminActionProposal *proposal,
                                CarminGuardDecision result, uint64_t now_ns);
const CarminGuardAudit *carmin_guard_audit_get(const CarminActuatorGuard *guard,
                                               size_t index_from_latest);

#endif

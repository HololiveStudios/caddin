#include "actuator_guard.h"

#include <math.h>
#include <string.h>

#define NS_PER_SECOND 1000000000ULL
#define CONFIRMATION_WINDOW_NS (5ULL * NS_PER_SECOND)

static void audit(CarminActuatorGuard *guard, const CarminActionProposal *proposal,
                  uint64_t now_ns, CarminGuardDecision decision) {
    CarminGuardAudit *entry = &guard->audit[guard->audit_head];
    *entry = (CarminGuardAudit){
        .sequence = guard->next_audit_sequence++,
        .timestamp_ns = now_ns,
        .proposal_id = proposal ? proposal->id : 0,
        .action = proposal ? proposal->action : UINT16_MAX,
        .decision = (uint16_t)decision,
        .flags = proposal ? proposal->flags : 0
    };
    guard->audit_head = (uint16_t)((guard->audit_head + 1u) % CARMIN_GUARD_AUDIT_CAPACITY);
    if (guard->audit_count < CARMIN_GUARD_AUDIT_CAPACITY) guard->audit_count++;
}

void carmin_guard_init(CarminActuatorGuard *guard) {
    if (!guard) return;
    memset(guard, 0, sizeof(*guard));
    guard->mode = CARMIN_MODE_SHADOW;
    guard->next_audit_sequence = 1;
}

bool carmin_guard_configure(CarminActuatorGuard *guard, CarminLearningMode mode,
                            uint32_t allow_mask, uint16_t max_actions_per_minute) {
    const uint32_t valid_mask = (1u << CARMIN_ACTION_COUNT) - 1u;
    if (!guard || (unsigned)mode > CARMIN_MODE_AUTONOMOUS || (allow_mask & ~valid_mask) ||
        max_actions_per_minute > CARMIN_GUARD_RATE_CAPACITY) return false;
    guard->mode = mode;
    guard->allow_mask = allow_mask;
    guard->max_actions_per_minute = max_actions_per_minute;
    guard->rate_count = 0;
    guard->rate_next = 0;
    guard->pending_valid = false;
    guard->approved_valid = false;
    return true;
}

static bool proposal_valid(const CarminActionProposal *p) {
    return p && p->version == CARMIN_GUARD_VERSION && p->size == sizeof(*p) &&
           p->id != 0 && p->action < CARMIN_ACTION_COUNT &&
           isfinite(p->confidence) && p->confidence >= 0.0f && p->confidence <= 1.0f &&
           p->expires_ns > p->created_ns;
}

static bool rate_available(CarminActuatorGuard *guard, uint64_t now_ns) {
    if (guard->max_actions_per_minute == 0) return false;
    uint64_t kept[CARMIN_GUARD_RATE_CAPACITY];
    uint16_t keep = 0;
    for (uint16_t i = 0; i < guard->rate_count; ++i) {
        uint16_t idx = (uint16_t)((guard->rate_next + CARMIN_GUARD_RATE_CAPACITY - guard->rate_count + i) %
                                  CARMIN_GUARD_RATE_CAPACITY);
        uint64_t at = guard->accepted_at[idx];
        if (now_ns < at || now_ns - at < 60ULL * NS_PER_SECOND) {
            kept[keep++] = at;
        }
    }
    memcpy(guard->accepted_at, kept, (size_t)keep * sizeof(*kept));
    guard->rate_count = keep;
    guard->rate_next = (uint16_t)(keep % CARMIN_GUARD_RATE_CAPACITY);
    return keep < guard->max_actions_per_minute;
}

static void record_accept(CarminActuatorGuard *guard, uint64_t now_ns) {
    guard->accepted_at[guard->rate_next] = now_ns;
    guard->rate_next = (uint16_t)((guard->rate_next + 1u) % CARMIN_GUARD_RATE_CAPACITY);
    if (guard->rate_count < CARMIN_GUARD_RATE_CAPACITY) guard->rate_count++;
}

static bool same_request(uint64_t id, uint64_t context, uint64_t target, uint16_t action,
                         const CarminActionProposal *p) {
    return p->id == id && p->context_token == context && p->target_token == target && p->action == action;
}

CarminGuardDecision carmin_guard_evaluate(CarminActuatorGuard *guard,
                                          const CarminActionProposal *proposal,
                                          uint64_t now_ns) {
    if (!guard) return CARMIN_GUARD_DENY_INVALID;
    if (!proposal_valid(proposal)) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_DENY_INVALID);
        return CARMIN_GUARD_DENY_INVALID;
    }
    if (proposal->created_ns > now_ns || proposal->expires_ns <= now_ns) {
        CarminGuardDecision result = CARMIN_GUARD_DENY_EXPIRED;
        audit(guard, proposal, now_ns, result);
        return result;
    }
    if (proposal->flags & (CARMIN_PROPOSAL_SENSITIVE | CARMIN_PROPOSAL_CRITICAL | CARMIN_PROPOSAL_PRIVATE)) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_DENY_PRIVACY);
        return CARMIN_GUARD_DENY_PRIVACY;
    }
    if (proposal->action == CARMIN_ACTION_OBSERVE) {
        CarminGuardDecision result = CARMIN_GUARD_OBSERVED;
        audit(guard, proposal, now_ns, result);
        return result;
    }
    bool needs_target = proposal->action != CARMIN_ACTION_INTERNAL_ADAPT &&
                        proposal->action != CARMIN_ACTION_STATUS_INDICATOR;
    if (needs_target && !(proposal->flags & CARMIN_PROPOSAL_TARGET_KNOWN)) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_DENY_UNKNOWN_TARGET);
        return CARMIN_GUARD_DENY_UNKNOWN_TARGET;
    }
    if (guard->mode == CARMIN_MODE_SHADOW) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_SHADOW_ONLY);
        return CARMIN_GUARD_SHADOW_ONLY;
    }
    if (!(guard->allow_mask & (1u << proposal->action))) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_DENY_NOT_ALLOWLISTED);
        return CARMIN_GUARD_DENY_NOT_ALLOWLISTED;
    }
    if (guard->mode == CARMIN_MODE_AUTONOMOUS &&
        !((proposal->action == CARMIN_ACTION_INTERNAL_ADAPT ||
           proposal->action == CARMIN_ACTION_STATUS_INDICATOR) &&
          (proposal->flags & CARMIN_PROPOSAL_REVERSIBLE))) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_DENY_MODE);
        return CARMIN_GUARD_DENY_MODE;
    }
    if (guard->mode == CARMIN_MODE_ASSISTED) {
        if (guard->approved_valid &&
            now_ns <= guard->pending_deadline_ns &&
            same_request(guard->approved_id, guard->approved_context, guard->approved_target,
                         guard->approved_action, proposal)) {
            guard->approved_valid = false;
        } else if (guard->pending_valid && now_ns <= guard->pending_deadline_ns &&
                   same_request(guard->pending_id, guard->pending_context, guard->pending_target,
                                guard->pending_action, proposal)) {
            audit(guard, proposal, now_ns, CARMIN_GUARD_CONFIRMATION_REQUIRED);
            return CARMIN_GUARD_CONFIRMATION_REQUIRED;
        } else {
            guard->pending_valid = true;
            guard->pending_id = proposal->id;
            guard->pending_context = proposal->context_token;
            guard->pending_target = proposal->target_token;
            guard->pending_action = proposal->action;
            uint64_t confirmation_deadline = now_ns + CONFIRMATION_WINDOW_NS;
            guard->pending_deadline_ns = proposal->expires_ns < confirmation_deadline ?
                proposal->expires_ns : confirmation_deadline;
            guard->approved_valid = false;
            audit(guard, proposal, now_ns, CARMIN_GUARD_CONFIRMATION_REQUIRED);
            return CARMIN_GUARD_CONFIRMATION_REQUIRED;
        }
    }
    if (!rate_available(guard, now_ns)) {
        audit(guard, proposal, now_ns, CARMIN_GUARD_DENY_RATE_LIMIT);
        return CARMIN_GUARD_DENY_RATE_LIMIT;
    }
    record_accept(guard, now_ns);
    guard->pending_valid = false;
    audit(guard, proposal, now_ns, CARMIN_GUARD_ALLOWED);
    return CARMIN_GUARD_ALLOWED;
}

bool carmin_guard_confirm(CarminActuatorGuard *guard, uint64_t proposal_id,
                          uint64_t context_token, uint64_t target_token,
                          CarminActionKind action, uint64_t now_ns) {
    if (!guard || guard->mode != CARMIN_MODE_ASSISTED || !guard->pending_valid ||
        now_ns > guard->pending_deadline_ns || guard->pending_id != proposal_id ||
        guard->pending_context != context_token || guard->pending_target != target_token ||
        guard->pending_action != action) return false;
    guard->approved_valid = true;
    guard->approved_id = proposal_id;
    guard->approved_context = context_token;
    guard->approved_target = target_token;
    guard->approved_action = (uint16_t)action;
    return true;
}

bool carmin_guard_cancel_pending(CarminActuatorGuard *guard, uint64_t proposal_id,
                                uint64_t now_ns) {
    if (!guard || !guard->pending_valid || guard->pending_id != proposal_id) return false;
    CarminActionProposal cancelled = {
        .id = guard->pending_id,
        .action = guard->pending_action,
        .flags = 0
    };
    guard->pending_valid = false;
    guard->approved_valid = false;
    audit(guard, &cancelled, now_ns, CARMIN_GUARD_DENY_CONFIRMATION);
    return true;
}

bool carmin_guard_record_result(CarminActuatorGuard *guard,
                                const CarminActionProposal *proposal,
                                CarminGuardDecision result, uint64_t now_ns) {
    if (!guard || !proposal_valid(proposal) ||
        (result != CARMIN_GUARD_RESULT_CANCELLED && result != CARMIN_GUARD_RESULT_FAILED) ||
        proposal->created_ns > now_ns || proposal->expires_ns <= now_ns) return false;
    audit(guard, proposal, now_ns, result);
    return true;
}

const CarminGuardAudit *carmin_guard_audit_get(const CarminActuatorGuard *guard,
                                               size_t index_from_latest) {
    if (!guard || index_from_latest >= guard->audit_count) return NULL;
    size_t index = (guard->audit_head + CARMIN_GUARD_AUDIT_CAPACITY - 1u - index_from_latest) %
                   CARMIN_GUARD_AUDIT_CAPACITY;
    return &guard->audit[index];
}

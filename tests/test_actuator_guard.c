#include "actuator_guard.h"

#include <assert.h>
#include <stdio.h>

#define SECOND 1000000000ULL

static CarminActionProposal proposal(uint64_t id, CarminActionKind action, uint64_t now,
                                     uint32_t flags) {
    return (CarminActionProposal){
        .version = CARMIN_GUARD_VERSION,
        .size = (uint16_t)sizeof(CarminActionProposal),
        .id = id,
        .context_token = 0x1234,
        .target_token = 0x5678,
        .created_ns = now,
        .expires_ns = now + 10 * SECOND,
        .action = (uint16_t)action,
        .flags = flags,
        .confidence = 0.9f
    };
}

static void test_shadow_privacy_and_audit(void) {
    CarminActuatorGuard guard;
    carmin_guard_init(&guard);
    assert(guard.mode == CARMIN_MODE_SHADOW);
    CarminActionProposal p = proposal(1, CARMIN_ACTION_CLICK, SECOND,
                                      CARMIN_PROPOSAL_TARGET_KNOWN);
    assert(carmin_guard_evaluate(&guard, &p, SECOND) == CARMIN_GUARD_SHADOW_ONLY);
    p.id++;
    p.flags |= CARMIN_PROPOSAL_PRIVATE;
    assert(carmin_guard_evaluate(&guard, &p, SECOND) == CARMIN_GUARD_DENY_PRIVACY);
    const CarminGuardAudit *entry = carmin_guard_audit_get(&guard, 0);
    assert(entry && entry->proposal_id == p.id && entry->decision == CARMIN_GUARD_DENY_PRIVACY);
    CarminActionProposal observation = proposal(3, CARMIN_ACTION_OBSERVE, SECOND, 0);
    assert(carmin_guard_evaluate(&guard, &observation, SECOND) == CARMIN_GUARD_OBSERVED);
    assert(carmin_guard_record_result(&guard, &observation,
                                      CARMIN_GUARD_RESULT_CANCELLED, SECOND));
    assert(carmin_guard_audit_get(&guard, 0)->decision == CARMIN_GUARD_RESULT_CANCELLED);
    assert(!carmin_guard_record_result(&guard, &observation, CARMIN_GUARD_ALLOWED, SECOND));
}

static void test_assisted_confirmation_is_bound_and_one_shot(void) {
    CarminActuatorGuard guard;
    carmin_guard_init(&guard);
    uint32_t mask = 1u << CARMIN_ACTION_FILE_WRITE;
    assert(carmin_guard_configure(&guard, CARMIN_MODE_ASSISTED, mask, 2));
    CarminActionProposal p = proposal(5, CARMIN_ACTION_FILE_WRITE, 2 * SECOND,
                                      CARMIN_PROPOSAL_TARGET_KNOWN | CARMIN_PROPOSAL_REVERSIBLE);
    assert(carmin_guard_evaluate(&guard, &p, 2 * SECOND) == CARMIN_GUARD_CONFIRMATION_REQUIRED);
    assert(!carmin_guard_cancel_pending(&guard, p.id + 1, 2 * SECOND));
    assert(carmin_guard_cancel_pending(&guard, p.id, 2 * SECOND));
    assert(carmin_guard_evaluate(&guard, &p, 2 * SECOND) == CARMIN_GUARD_CONFIRMATION_REQUIRED);
    assert(!carmin_guard_confirm(&guard, p.id, p.context_token, p.target_token + 1,
                                 CARMIN_ACTION_FILE_WRITE, 2 * SECOND));
    assert(carmin_guard_confirm(&guard, p.id, p.context_token, p.target_token,
                                CARMIN_ACTION_FILE_WRITE, 2 * SECOND));
    assert(carmin_guard_evaluate(&guard, &p, 2 * SECOND) == CARMIN_GUARD_ALLOWED);
    assert(carmin_guard_evaluate(&guard, &p, 2 * SECOND) == CARMIN_GUARD_CONFIRMATION_REQUIRED);
}

static void test_expiry_unknown_targets_rate_and_autonomous_limits(void) {
    CarminActuatorGuard guard;
    carmin_guard_init(&guard);
    uint32_t mask = 1u << CARMIN_ACTION_CURSOR_MOVE;
    assert(carmin_guard_configure(&guard, CARMIN_MODE_ASSISTED, mask, 1));
    CarminActionProposal expired = proposal(9, CARMIN_ACTION_CURSOR_MOVE, SECOND,
                                           CARMIN_PROPOSAL_TARGET_KNOWN);
    expired.expires_ns = 2 * SECOND;
    assert(carmin_guard_evaluate(&guard, &expired, 2 * SECOND) == CARMIN_GUARD_DENY_EXPIRED);
    CarminActionProposal p = proposal(10, CARMIN_ACTION_CURSOR_MOVE, 3 * SECOND, 0);
    assert(carmin_guard_evaluate(&guard, &p, 3 * SECOND) == CARMIN_GUARD_DENY_UNKNOWN_TARGET);
    p.flags = CARMIN_PROPOSAL_TARGET_KNOWN | CARMIN_PROPOSAL_REVERSIBLE;
    assert(carmin_guard_evaluate(&guard, &p, 3 * SECOND) == CARMIN_GUARD_CONFIRMATION_REQUIRED);
    assert(carmin_guard_confirm(&guard, p.id, p.context_token, p.target_token,
                                CARMIN_ACTION_CURSOR_MOVE, 3 * SECOND));
    assert(carmin_guard_evaluate(&guard, &p, 3 * SECOND) == CARMIN_GUARD_ALLOWED);
    p.id++;
    p.created_ns += SECOND;
    p.expires_ns = p.created_ns + SECOND;
    assert(carmin_guard_evaluate(&guard, &p, 4 * SECOND) == CARMIN_GUARD_CONFIRMATION_REQUIRED);
    assert(carmin_guard_confirm(&guard, p.id, p.context_token, p.target_token,
                                CARMIN_ACTION_CURSOR_MOVE, 4 * SECOND));
    assert(carmin_guard_evaluate(&guard, &p, 4 * SECOND) == CARMIN_GUARD_DENY_RATE_LIMIT);

    assert(carmin_guard_configure(&guard, CARMIN_MODE_AUTONOMOUS,
                                  (1u << CARMIN_ACTION_INTERNAL_ADAPT) |
                                  (1u << CARMIN_ACTION_SHELL), 4));
    CarminActionProposal adapt = proposal(20, CARMIN_ACTION_INTERNAL_ADAPT, 5 * SECOND,
                                         CARMIN_PROPOSAL_REVERSIBLE);
    assert(carmin_guard_evaluate(&guard, &adapt, 5 * SECOND) == CARMIN_GUARD_ALLOWED);
    CarminActionProposal shell = proposal(21, CARMIN_ACTION_SHELL, 5 * SECOND,
                                          CARMIN_PROPOSAL_TARGET_KNOWN);
    assert(carmin_guard_evaluate(&guard, &shell, 5 * SECOND) == CARMIN_GUARD_DENY_MODE);
}

int main(void) {
    test_shadow_privacy_and_audit();
    test_assisted_confirmation_is_bound_and_one_shot();
    test_expiry_unknown_targets_rate_and_autonomous_limits();
    puts("Actuator Guard: OK");
    return 0;
}

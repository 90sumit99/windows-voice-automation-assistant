#pragma once
#include "Intent.h"
#include <unordered_set>

enum class PolicyLevel {
    POLICY_NORMAL,   // standard user - most actions allowed
    POLICY_STRICT,   // restrict destructive actions
    POLICY_LOCKED    // block everything except exit
};

class PolicyEngine {
public:
    PolicyEngine(PolicyLevel level = PolicyLevel::POLICY_NORMAL);

    bool isAllowed(const Intent& intent) const;

    void        setLevel(PolicyLevel level);
    PolicyLevel getLevel() const { return currentLevel; }

    static bool isRunningAsAdmin();

private:
    PolicyLevel currentLevel;

    const std::unordered_set<IntentType> adminRequired = {
        IntentType::SHUTDOWN,
        IntentType::RESTART,
    };

    const std::unordered_set<IntentType> strictBlocked = {
        IntentType::SHUTDOWN,
        IntentType::RESTART,
        IntentType::DELETE_FILE,
        IntentType::RUN_COMMAND,
    };
};

// Include windows.h FIRST, then kill its conflicting macros
#include <windows.h>
#undef NORMAL
#undef STRICT
#undef LOCKED
#undef DELETE

#include "PolicyEngine.h"
#include <iostream>

// ─────────────────────────────────────────────────────────────
//  CONSTRUCTOR
// ─────────────────────────────────────────────────────────────
PolicyEngine::PolicyEngine(PolicyLevel level)
    : currentLevel(level) {
}

void PolicyEngine::setLevel(PolicyLevel level) {
    currentLevel = level;
}

// ─────────────────────────────────────────────────────────────
//  ADMIN CHECK
// ─────────────────────────────────────────────────────────────
bool PolicyEngine::isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(
            &ntAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

// ─────────────────────────────────────────────────────────────
//  IS ALLOWED
// ─────────────────────────────────────────────────────────────
bool PolicyEngine::isAllowed(const Intent& intent) const {

    // LOCKED mode - block everything except exit
    if (currentLevel == PolicyLevel::POLICY_LOCKED) {
        if (intent.type == IntentType::EXIT_APP) return true;
        std::cout << "[Policy] ZYREX is locked. All commands blocked.\n";
        return false;
    }

    // STRICT mode - block destructive commands
    if (currentLevel == PolicyLevel::POLICY_STRICT) {
        if (strictBlocked.count(intent.type)) {
            std::cout << "[Policy] Blocked in strict mode.\n";
            return false;
        }
    }

    // Admin-required commands
    if (adminRequired.count(intent.type)) {
        if (!isRunningAsAdmin()) {
            std::cout << "[Policy] This command requires admin privileges.\n";
            std::cout << "         Restart ZYREX as Administrator to use it.\n";
            return false;
        }
    }

    // DELETE FILE - always warn before action
    if (intent.type == IntentType::DELETE_FILE) {
        std::cout << "[Policy] Delete operation will ask for confirmation.\n";
    }

    return true;
}

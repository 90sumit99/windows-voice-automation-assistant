#include "PolicyEngine.h"

bool PolicyEngine::isAllowed(const Intent&) const {
    // Allow everything for now
    return true;
}

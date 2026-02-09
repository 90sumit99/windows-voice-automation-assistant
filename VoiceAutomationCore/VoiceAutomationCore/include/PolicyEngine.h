#pragma once
#include "Intent.h"

class PolicyEngine {
public:
    bool isAllowed(const Intent& intent) const;
};

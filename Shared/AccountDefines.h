#pragma once

#include <stdint.h>

namespace AccountDefines
{
    // Input validation limits
    constexpr int MaxUserLength = 32;
    constexpr int MaxPassLength = 64;

    // Authentication result codes
    enum class AuthenticateResult : uint8_t
    {
        Validated = 0,      // Success - proceed to character select
        WrongVersion = 1,   // Client needs update
        BadPassword = 2,    // Invalid token/credentials
        ServerFull = 3,     // Server at capacity
        Banned = 4,         // Account suspended
    };
}

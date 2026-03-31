#pragma once

#include <string>

enum class AuthPath {
    AKA,
    Digest,
};

struct Challenge {
    std::string scheme;
    std::string algorithm;
    std::string realm;
    std::string nonce;
};

struct AuthContext {
    bool hasAkaMaterial{};
    bool fallbackUsed{};
    std::string digestPassword;
};

struct AuthResult {
    AuthPath path;
    std::string authorizationHeader;
};

class ImsAuthService {
public:
    AuthResult BuildAuthorization(const Challenge& ch, const AuthContext& ctx) const;
};

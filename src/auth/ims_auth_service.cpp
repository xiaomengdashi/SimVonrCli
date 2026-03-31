#include "auth/ims_auth_service.hpp"

#include <cctype>
#include <stdexcept>

namespace {

std::string ToUpper(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool IsAkaAlgorithm(const std::string& algorithm) {
    const auto upper = ToUpper(algorithm);
    return upper.rfind("AKAV1-", 0) == 0 || upper.rfind("AKAV2-", 0) == 0;
}

std::string QuoteParamValue(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 2);
    for (char ch : value) {
        if (ch == '\r' || ch == '\n') {
            throw std::runtime_error("invalid auth parameter");
        }
        if (ch == '\\' || ch == '"') {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    return "\"" + out + "\"";
}

std::string BuildAuthorizationHeader(const std::string& algorithm, const Challenge& ch) {
    return "Authorization: Digest algorithm=" + QuoteParamValue(algorithm) +
           ", realm=" + QuoteParamValue(ch.realm) +
           ", nonce=" + QuoteParamValue(ch.nonce);
}

} // namespace

AuthResult ImsAuthService::BuildAuthorization(const Challenge& ch, const AuthContext& ctx) const {
    if (ToUpper(ch.scheme) != "DIGEST") {
        throw std::runtime_error("unsupported auth scheme");
    }

    if (IsAkaAlgorithm(ch.algorithm) && ctx.hasAkaMaterial) {
        const auto algorithm = ch.algorithm.empty() ? "AKAv1-MD5" : ch.algorithm;
        return {AuthPath::AKA, BuildAuthorizationHeader(algorithm, ch)};
    }

    if (IsAkaAlgorithm(ch.algorithm) && !ctx.hasAkaMaterial) {
        if (ctx.fallbackUsed) {
            throw std::runtime_error("fallback already used");
        }
        if (ctx.digestPassword.empty()) {
            throw std::runtime_error("digest password missing");
        }
        return {AuthPath::Digest, BuildAuthorizationHeader("MD5", ch)};
    }

    if (ctx.digestPassword.empty()) {
        throw std::runtime_error("digest password missing");
    }

    const auto algorithm = ch.algorithm.empty() ? "MD5" : ch.algorithm;
    return {AuthPath::Digest, BuildAuthorizationHeader(algorithm, ch)};
}

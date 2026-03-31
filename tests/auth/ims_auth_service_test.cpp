#include <stdexcept>

#include <gtest/gtest.h>

#include "auth/ims_auth_service.hpp"

TEST(ImsAuthServiceTest, PreferAkaWhenChallengeIsAka) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Digest", .algorithm = "AKAv1-MD5", .realm = "ims.test", .nonce = "n"};

    auto out = svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial = true, .digestPassword = "pw"});

    EXPECT_EQ(out.path, AuthPath::AKA);
    EXPECT_NE(out.authorizationHeader.find("algorithm=\"AKAv1-MD5\""), std::string::npos);
    EXPECT_NE(out.authorizationHeader.find("realm=\"ims.test\""), std::string::npos);
    EXPECT_NE(out.authorizationHeader.find("nonce=\"n\""), std::string::npos);
}

TEST(ImsAuthServiceTest, FallbackToDigestWhenAkaUnavailable) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Digest", .algorithm = "AKAv1-MD5", .realm = "ims.test", .nonce = "n"};

    auto out = svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial = false, .digestPassword = "pw"});

    EXPECT_EQ(out.path, AuthPath::Digest);
    EXPECT_NE(out.authorizationHeader.find("algorithm=\"MD5\""), std::string::npos);
}

TEST(ImsAuthServiceTest, PreventInfiniteFallbackLoop) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Digest", .algorithm = "AKAv1-MD5", .realm = "ims.test", .nonce = "n"};
    AuthContext ctx{.hasAkaMaterial = false, .fallbackUsed = true, .digestPassword = "pw"};

    EXPECT_THROW(svc.BuildAuthorization(ch, ctx), std::runtime_error);
}

TEST(ImsAuthServiceTest, UseDigestForNonAkaChallenge) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Digest", .algorithm = "MD5", .realm = "ims.test", .nonce = "n"};

    auto out = svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial = true, .digestPassword = "pw"});

    EXPECT_EQ(out.path, AuthPath::Digest);
    EXPECT_NE(out.authorizationHeader.find("algorithm=\"MD5\""), std::string::npos);
}

TEST(ImsAuthServiceTest, ThrowWhenDigestPasswordMissingForNonAka) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Digest", .algorithm = "MD5", .realm = "ims.test", .nonce = "n"};

    EXPECT_THROW(svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial = false, .digestPassword = ""}), std::runtime_error);
}

TEST(ImsAuthServiceTest, ThrowWhenSchemeIsNotDigest) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Basic", .algorithm = "MD5", .realm = "ims.test", .nonce = "n"};

    EXPECT_THROW(svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial = true, .digestPassword = "pw"}), std::runtime_error);
}

TEST(ImsAuthServiceTest, EscapeQuotedAuthParameters) {
    ImsAuthService svc;
    Challenge ch{.scheme = "Digest", .algorithm = "MD5", .realm = "ims\"test", .nonce = "n\\once"};

    auto out = svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial = true, .digestPassword = "pw"});

    EXPECT_NE(out.authorizationHeader.find("realm=\"ims\\\"test\""), std::string::npos);
    EXPECT_NE(out.authorizationHeader.find("nonce=\"n\\\\once\""), std::string::npos);
}

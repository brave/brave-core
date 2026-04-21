/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/manifest_verifier.h"

#include "base/base64url.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace webcat {

namespace {

std::string Base64UrlSha256(const std::string& content) {
  auto digest = crypto::SHA256(base::as_byte_span(content));
  std::string encoded;
  base::Base64UrlEncode(base::as_chars(digest),
                        base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded);
  return encoded;
}

Manifest MakeMinimalManifest() {
  Manifest m;
  m.app = "https://app.eth";
  m.version = "1.0.0";
  m.default_csp = "default-src 'self'; object-src 'none'";
  m.default_index = "/index.html";
  m.default_fallback = "/index.html";
  m.files["/index.html"] = Base64UrlSha256("<html>hello</html>");
  m.files["/app.js"] = Base64UrlSha256("console.log('app');");
  return m;
}

}  // namespace

TEST(ManifestVerifierTest, VerifyValidBundle) {
  Bundle bundle;
  bundle.manifest = MakeMinimalManifest();
  bundle.signatures["pubkey1"] = "sig1";

  auto result = VerifyBundle(bundle, "somecid");
  EXPECT_TRUE(result.success);
}

TEST(ManifestVerifierTest, VerifyInvalidDefaultCsp) {
  Bundle bundle;
  bundle.manifest = MakeMinimalManifest();
  bundle.manifest.default_csp = "default-src 'self'; script-src 'unsafe-eval'";
  bundle.signatures["pubkey1"] = "sig1";

  auto result = VerifyBundle(bundle, "somecid");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error, WebcatError::kCspValidationFailed);
}

TEST(ManifestVerifierTest, VerifyInvalidExtraCsp) {
  Bundle bundle;
  bundle.manifest = MakeMinimalManifest();
  bundle.manifest.extra_csp["/admin/"] = "default-src *; script-src *";
  bundle.signatures["pubkey1"] = "sig1";

  auto result = VerifyBundle(bundle, "somecid");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error, WebcatError::kCspValidationFailed);
}

TEST(ManifestVerifierTest, VerifyContentHashMatch) {
  std::string content = "<html>hello world</html>";
  std::string expected_hash = Base64UrlSha256(content);

  auto result = VerifyContentHash(content, expected_hash);
  EXPECT_TRUE(result.success);
}

TEST(ManifestVerifierTest, VerifyContentHashMismatch) {
  std::string content = "<html>hello world</html>";
  std::string wrong_hash = Base64UrlSha256("different content");

  auto result = VerifyContentHash(content, wrong_hash);
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error, WebcatError::kContentIntegrityFailed);
}

TEST(ManifestVerifierTest, VerifyContentHashEmptyContent) {
  std::string content;
  std::string expected_hash = Base64UrlSha256(content);

  auto result = VerifyContentHash(content, expected_hash);
  EXPECT_TRUE(result.success);
}

TEST(ManifestVerifierTest, VerifyContentHashWithPadding) {
  std::string content = "test";
  auto digest = crypto::SHA256(base::as_byte_span(content));
  std::string encoded_with_padding;
  base::Base64UrlEncode(base::as_chars(digest),
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &encoded_with_padding);

  std::string encoded_without_padding;
  base::Base64UrlEncode(base::as_chars(digest),
                        base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &encoded_without_padding);

  auto result = VerifyContentHash(content, encoded_without_padding);
  EXPECT_TRUE(result.success);
}

}  // namespace webcat
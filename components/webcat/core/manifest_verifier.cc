/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/manifest_verifier.h"

#include <cstdint>
#include <vector>

#include "base/base64url.h"
#include "crypto/sha2.h"

namespace webcat {

namespace {

std::string Base64UrlEncode(base::span<const uint8_t> data) {
  std::string encoded;
  base::Base64UrlEncode(
      base::as_chars(data),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded);
  return encoded;
}

}  // namespace

VerificationResult VerifyBundle(const Bundle& bundle,
                                const std::string& expected_cid) {
  VerificationResult result;

  auto csp_result = ValidateDefaultCsp(bundle.manifest.default_csp);
  if (!csp_result.is_valid) {
    result.error = WebcatError::kCspValidationFailed;
    result.error_detail = "default_csp validation failed: " + csp_result.error_detail;
    return result;
  }

  for (const auto& [prefix, csp] : bundle.manifest.extra_csp) {
    auto extra_result = ValidateCsp(csp);
    if (!extra_result.is_valid) {
      result.error = WebcatError::kCspValidationFailed;
      result.error_detail = "extra_csp[" + prefix + "] validation failed: " +
                            extra_result.error_detail;
      return result;
    }
  }

  result.success = true;
  return result;
}

VerificationResult VerifyContentHash(const std::string& content,
                                     const std::string& expected_hash) {
  VerificationResult result;

  std::array<uint8_t, crypto::kSHA256Length> digest =
      crypto::SHA256(base::as_byte_span(content));

  std::string actual_hash = Base64UrlEncode(digest);

  if (actual_hash != expected_hash) {
    result.error = WebcatError::kContentIntegrityFailed;
    result.error_detail = "Content hash mismatch: expected " + expected_hash +
                          ", got " + actual_hash;
    return result;
  }

  result.success = true;
  return result;
}

}  // namespace webcat

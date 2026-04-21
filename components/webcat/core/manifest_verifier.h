/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_MANIFEST_VERIFIER_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_MANIFEST_VERIFIER_H_

#include <optional>
#include <string>

#include "brave/components/webcat/core/bundle_parser.h"
#include "brave/components/webcat/core/constants.h"

namespace webcat {

struct VerificationResult {
  bool success = false;
  WebcatError error = WebcatError::kNone;
  std::string error_detail;
};

VerificationResult VerifyBundle(const Bundle& bundle,
                                const std::string& expected_cid);

VerificationResult VerifyContentHash(const std::string& content,
                                     const std::string& expected_hash);

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_MANIFEST_VERIFIER_H_

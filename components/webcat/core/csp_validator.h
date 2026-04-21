/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_CSP_VALIDATOR_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_CSP_VALIDATOR_H_

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "brave/components/webcat/core/constants.h"

namespace webcat {

struct CspValidationResult {
  bool is_valid = false;
  std::string error_detail;
  std::set<std::string> enrolled_origins;
};

CspValidationResult ValidateCsp(const std::string& csp_string);

CspValidationResult ValidateDefaultCsp(const std::string& csp_string);

std::string GetEffectiveCspForPath(const std::string& path,
                                   const std::string& default_csp,
                                   const std::map<std::string, std::string>& extra_csp);

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_CSP_VALIDATOR_H_

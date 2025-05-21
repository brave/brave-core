// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_opeartion_context.h"

#include <memory>
#include <optional>
#include <string>

#include "base/strings/string_util.h"
#include "brave/components/psst/common/constants.h"

namespace psst {

PsstOperationContext::PsstOperationContext() = default;
PsstOperationContext::~PsstOperationContext() = default;

void PsstOperationContext::SetUserScriptResult(
    const base::Value& user_script_result,
    const MatchedRule& rule) {
  if (!user_script_result.is_dict()) {
    return;
  }

  const auto* params = user_script_result.GetIfDict();

  if (const std::string* parsed_user_id =
          params->FindString(kUserScriptResultUserPropName)) {
    user_id_ = *parsed_user_id;
  }

  rule_name_ = rule.Name();
}

const std::optional<std::string>& PsstOperationContext::GetUserId() const {
  return user_id_;
}

const std::optional<std::string>& PsstOperationContext::GetRuleName() const {
  return rule_name_;
}

bool PsstOperationContext::IsUserScriptExecuted() const {
  return user_id_ && rule_name_;
}

bool PsstOperationContext::IsPolicyScriptExecuted() const {
  return false;
}

}  // namespace psst

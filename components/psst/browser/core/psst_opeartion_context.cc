// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_opeartion_context.h"

#include <memory>
#include <optional>
#include <string>

#include "base/strings/string_util.h"
#include "brave/components/psst/common/psst_constants.h"

namespace psst {

// static
std::unique_ptr<PsstOperationContext> PsstOperationContext::LoadContext(
    const base::Value& user_script_result,
    const MatchedRule& rule) {
  if (!user_script_result.is_dict()) {
    return nullptr;
  }

  const auto* params = user_script_result.GetIfDict();

  std::optional<std::string> user_id;
  if (const std::string* parsed_user_id =
          params->FindString(kUserScriptResultUserPropName)) {
    user_id = *parsed_user_id;
  }

  if (!user_id) {
    return nullptr;
  }

  return base::WrapUnique<PsstOperationContext>(
      new PsstOperationContext(*user_id, rule));
}

PsstOperationContext::PsstOperationContext(const std::string& user_id,
                                           const MatchedRule& rule)
    : user_id_(user_id), rule_name_(rule.Name()) {}

const std::string& PsstOperationContext::GetUserId() const {
  return user_id_;
}

const std::string& PsstOperationContext::GetRuleName() const {
  return rule_name_;
}

bool PsstOperationContext::IsValid() const {
  return !user_id_.empty() && !rule_name_.empty();
}

}  // namespace psst

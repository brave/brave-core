// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/matched_rule.h"

#include <memory>
#include <optional>
#include <string>

#include "base/memory/ptr_util.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"

namespace psst {

MatchedRule::MatchedRule(const std::string& name,
                         const std::string& user_script,
                         const std::string& policy_script,
                         int version)
    : name_(name),
      user_script_(user_script),
      policy_script_(policy_script),
      version_(version) {}

MatchedRule::~MatchedRule() = default;

// static
std::unique_ptr<MatchedRule> MatchedRule::Create(
    std::unique_ptr<RuleDataReader> rule_reader,
    const PsstRule& rule) {
  CHECK(rule_reader);

  auto user_script =
      rule_reader->ReadUserScript(rule.name(), rule.user_script_path());
  auto policy_script =
      rule_reader->ReadPolicyScript(rule.name(), rule.policy_script_path());

  if (!user_script || !policy_script) {
    return nullptr;
  }

  return base::WrapUnique<MatchedRule>(new MatchedRule(
      rule.name(), user_script.value(), policy_script.value(), rule.version()));
}

}  // namespace psst

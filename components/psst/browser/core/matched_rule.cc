// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/matched_rule.h"

#include <optional>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"

namespace psst {

MatchedRule::MatchedRule(const std::string& name,
                         const std::string& user_script,
                         const std::string& test_script,
                         const std::string& policy_script,
                         int version)
    : name_(name),
      user_script_(user_script),
      test_script_(test_script),
      policy_script_(policy_script),
      version_(version) {}

MatchedRule::~MatchedRule() = default;
MatchedRule::MatchedRule(const MatchedRule&) = default;

MatchedRuleFactory::MatchedRuleFactory(
    RuleDataReader* rule_reader,
    const std::string& rule_name,
    const int version)
    : rule_reader_(rule_reader),
      name_(rule_name),
      version_(version) {}

std::optional<MatchedRule> MatchedRuleFactory::Create(const PsstRule& rule) {
  DCHECK(rule_reader_);
  if(!rule_reader_) {
    return std::nullopt;
  }

  auto user_script = rule_reader_->ReadUserScript(rule);
  auto test_script = rule_reader_->ReadTestScript(rule);
  auto policy_script = rule_reader_->ReadPolicyScript(rule);

  if (!user_script || !test_script || !policy_script) {
    return std::nullopt;
  }

  return MatchedRule(name_, user_script.value(), test_script.value(),
                     policy_script.value(), version_);
}

}  // namespace psst

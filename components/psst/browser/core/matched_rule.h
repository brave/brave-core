// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "brave/components/psst/browser/core/psst_rule.h"

namespace psst {

// Used to hold the loaded script contents for a matched PsstRule.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) MatchedRule {
 public:
  ~MatchedRule();
  MatchedRule(const MatchedRule&);
  MatchedRule& operator=(const MatchedRule&) = delete;

  // Getters.
  const std::string& UserScript() const { return user_script_; }
  const std::string& TestScript() const { return test_script_; }
  const std::string& PolicyScript() const { return policy_script_; }
  int Version() const { return version_; }
  const std::string& Name() const { return name_; }

 private:
  friend class MatchedRuleFactory;
  friend class RuleDataReaderUnitTest;
  MatchedRule(const std::string& name,
              const std::string& user_script,
              const std::string& test_script,
              const std::string& policy_script,
              int version);

  const std::string name_;
  const std::string user_script_;
  const std::string test_script_;
  const std::string policy_script_;
  int version_;
};

class RuleDataReader;
class COMPONENT_EXPORT(PSST_BROWSER_CORE) MatchedRuleFactory {
public:
  explicit MatchedRuleFactory(RuleDataReader* rule_reader, const std::string& rule_name, const int version);
  MatchedRuleFactory(const MatchedRuleFactory&) = delete;
  MatchedRuleFactory& operator=(const MatchedRuleFactory&) = delete;
  virtual ~MatchedRuleFactory() = default;

  virtual std::optional<MatchedRule> Create(const PsstRule& rule);
private:
  raw_ptr<RuleDataReader> rule_reader_;
  const std::string name_;
  const int version_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_

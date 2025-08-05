// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_

#include <memory>
#include <string>

#include "brave/components/psst/browser/core/psst_rule.h"

namespace psst {

class RuleDataReader;

// Represents the loaded PSST data for PsstRule matched by the URL.
class MatchedRule {
 public:
  ~MatchedRule();
  MatchedRule(const MatchedRule&) = delete;
  MatchedRule& operator=(const MatchedRule&) = delete;

  static std::unique_ptr<MatchedRule> Create(
      std::unique_ptr<RuleDataReader> rule_reader,
      const PsstRule& rule);

  // Getters.
  const std::string& user_script() const { return user_script_; }
  const std::string& policy_script() const { return policy_script_; }
  int version() const { return version_; }
  const std::string& name() const { return name_; }

 private:
  friend class RuleDataReaderUnitTest;
  friend class PsstTabWebContentsObserverUnitTestBase;
  MatchedRule(const std::string& name,
              const std::string& user_script,
              const std::string& policy_script,
              int version);

  const std::string name_;
  const std::string user_script_;
  const std::string policy_script_;
  int version_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_

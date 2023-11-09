// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_

#include <string>

#include "base/component_export.h"
#include "base/files/file_path.h"

namespace psst {

// Used to hold the loaded script contents for a matched PsstRule.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) MatchedRule {
 public:
  static const MatchedRule CreateMatchedRule(
      const base::FilePath& component_path,
      const std::string& name,
      const base::FilePath& user_script_path,
      const base::FilePath& test_script_path,
      const base::FilePath& policy_script_path,
      const int version);

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

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_MATCHED_RULE_H_

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_RULE_DATA_READER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_RULE_DATA_READER_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"

namespace psst {

// Represents the reader of the rule data files (user.js and policy.js) for a
// given rule. The data files are stored in the component directory under
// "scripts/<rule_name>/user.js" and "scripts/<rule_name>/policy.js".
class RuleDataReader {
 public:
  explicit RuleDataReader(const base::FilePath& component_path);
  RuleDataReader(const RuleDataReader&) = delete;
  RuleDataReader& operator=(const RuleDataReader&) = delete;
  ~RuleDataReader() = default;

  std::optional<std::string> ReadUserScript(
      const std::string& rule_name,
      const base::FilePath& user_script_path) const;
  std::optional<std::string> ReadPolicyScript(
      const std::string& rule_name,
      const base::FilePath& policy_script_path) const;

 private:
  base::FilePath prefix_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_RULE_DATA_READER_H_

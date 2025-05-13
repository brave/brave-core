// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_RULE_DATA_READER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_RULE_DATA_READER_H_

#include <optional>
#include <string>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "brave/components/psst/browser/core/psst_rule.h"

namespace psst {

class COMPONENT_EXPORT(PSST_BROWSER_CORE) RuleDataReader {
 public:
  explicit RuleDataReader(const base::FilePath& component_path);
  RuleDataReader(const RuleDataReader&) = delete;
  RuleDataReader& operator=(const RuleDataReader&) = delete;
  virtual ~RuleDataReader() = default;

  virtual std::optional<std::string> ReadUserScript(const PsstRule& rule) const;
  virtual std::optional<std::string> ReadPolicyScript(
      const PsstRule& rule) const;

 private:
  base::FilePath prefix_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_RULE_DATA_READER_H_

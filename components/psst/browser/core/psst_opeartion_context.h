// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_OPEARTION_CONTEXT_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_OPEARTION_CONTEXT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/component_export.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "url/gurl.h"

namespace psst {

// Represents the context of the PSST operation, allows to save and load the
// context
class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstOperationContext {
 public:
  PsstOperationContext();

  PsstOperationContext(const PsstOperationContext&) = delete;
  PsstOperationContext(PsstOperationContext&&) = delete;
  PsstOperationContext& operator=(const PsstOperationContext&) = delete;
  PsstOperationContext& operator=(PsstOperationContext&&) = delete;
  ~PsstOperationContext();

  void SetUserScriptResult(const base::Value& user_script_result,
                           const MatchedRule& rule);

  const std::optional<std::string>& GetUserId() const;
  const std::optional<std::string>& GetRuleName() const;

  bool IsUserScriptExecuted() const;

  bool IsPolicyScriptExecuted() const;

 private:
  std::optional<std::string> user_id_;
  std::optional<std::string> rule_name_;
};
}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_OPEARTION_CONTEXT_H_

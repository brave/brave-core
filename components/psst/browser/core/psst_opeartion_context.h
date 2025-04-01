// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_OPERATION_CONTEXT_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_OPERATION_CONTEXT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/component_export.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "url/gurl.h"

namespace psst {

class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstOperationContext {
 public:
  PsstOperationContext(const PsstOperationContext&) = delete;
  PsstOperationContext(PsstOperationContext&&) = delete;
  PsstOperationContext& operator=(const PsstOperationContext&) = delete;
  PsstOperationContext& operator=(PsstOperationContext&&) = delete;
  ~PsstOperationContext() = default;

  static std::unique_ptr<PsstOperationContext> LoadContext(
      const base::Value& user_script_result,
      const MatchedRule& rule);

  std::string GetUserId() const;
  std::string GetRuleName() const;

  bool IsValid() const;

  const std::optional<GURL> GetShareLink(
      const std::u16string& pre_populate_text) const;

 private:
  explicit PsstOperationContext(const std::string& user_id,
                                const std::string& share_experience_link,
                                const MatchedRule& rule);

  std::string user_id_;
  std::string share_experience_link_;
  std::string rule_name_;
};
}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_OPERATION_CONTEXT_H_

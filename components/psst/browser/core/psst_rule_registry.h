// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_REGISTRY_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_REGISTRY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "url/gurl.h"

namespace psst {

// Represents the registry of PSST rules.
// It allows to load the all items from the psst.json file and match them
// against the URL. For matched rules, it loads rule data (the user.js and
// policy.js script contents) with using of rule data reader.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstRuleRegistry {
 public:
  using OnLoadCallback = base::OnceCallback<void(const std::string&,
                                                 const std::vector<PsstRule>&)>;
  static PsstRuleRegistry* GetInstance();

  // Returns the matched PSST rule, if any.
  virtual void CheckIfMatch(
      const GURL& url,
      base::OnceCallback<void(std::unique_ptr<MatchedRule>)> cb) = 0;

  virtual void LoadRules(const base::FilePath& path, OnLoadCallback cb) = 0;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_REGISTRY_H_

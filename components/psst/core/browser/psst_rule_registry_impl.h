// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_RULE_REGISTRY_IMPL_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_RULE_REGISTRY_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "brave/components/psst/core/browser/matched_rule.h"
#include "brave/components/psst/core/browser/psst_rule.h"
#include "brave/components/psst/core/browser/psst_rule_registry.h"
#include "url/gurl.h"

namespace psst {

class PsstRuleRegistryImpl : public PsstRuleRegistry {
 public:
  ~PsstRuleRegistryImpl();

  // PsstRuleRegistry overrides
  void CheckIfMatch(
      const GURL& url,
      base::OnceCallback<void(std::unique_ptr<MatchedRule>)> cb) override;
  void LoadRules(const base::FilePath& path, OnLoadCallback cb) override;

 private:
  PsstRuleRegistryImpl();
  friend base::NoDestructor<PsstRuleRegistryImpl>;

  friend class PsstRuleRegistryUnitTest;

  void OnLoadRules(OnLoadCallback cb, const std::string& data);

  std::vector<PsstRule> rules_;
  base::FilePath component_path_;
  base::WeakPtrFactory<PsstRuleRegistryImpl> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_RULE_REGISTRY_IMPL_H_

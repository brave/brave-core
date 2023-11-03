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
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace psst {

FORWARD_DECLARE_TEST(PsstTabHelperBrowserTest, NoMatch);
FORWARD_DECLARE_TEST(PsstTabHelperBrowserTest, RuleMatchTestScriptFalse);
FORWARD_DECLARE_TEST(PsstTabHelperBrowserTest, RuleMatchTestScriptTrue);

// This class loads and stores the rules from the psst.json file.
// It is also used for matching based on the URL.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstRuleRegistry {
 public:
  PsstRuleRegistry(const PsstRuleRegistry&) = delete;
  PsstRuleRegistry& operator=(const PsstRuleRegistry&) = delete;
  ~PsstRuleRegistry();
  void CheckIfMatch(const GURL& url,
                    base::OnceCallback<void(MatchedRule)> cb) const;
  static PsstRuleRegistry* GetInstance();  // singleton
  void LoadRules(const base::FilePath& path);

 private:
  PsstRuleRegistry();
  // These methods are also called by PsstTabHelperBrowserTest.
  void OnLoadRules(const std::string& data);
  void SetComponentPath(const base::FilePath& path);

  base::FilePath component_path_;
  std::vector<PsstRule> rules_;

  base::WeakPtrFactory<PsstRuleRegistry> weak_factory_{this};

  FRIEND_TEST_ALL_PREFIXES(PsstTabHelperBrowserTest, NoMatch);
  FRIEND_TEST_ALL_PREFIXES(PsstTabHelperBrowserTest, RuleMatchTestScriptFalse);
  FRIEND_TEST_ALL_PREFIXES(PsstTabHelperBrowserTest, RuleMatchTestScriptTrue);
  friend class PsstTabHelperBrowserTest;  // Used for testing private methods.
  friend struct base::DefaultSingletonTraits<PsstRuleRegistry>;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_REGISTRY_H_

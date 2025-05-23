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
#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"

class GURL;

namespace psst {

// Represents the registry of PSST rules.
// It allows to load the all items from the psst.json file and match them
// against the URL. For matched rules, it loads rule data (the user.js and
// policy.js script contents) with using of rule data reader.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstRuleRegistry {
 public:
  PsstRuleRegistry(const PsstRuleRegistry&) = delete;
  PsstRuleRegistry& operator=(const PsstRuleRegistry&) = delete;

  ~PsstRuleRegistry();

  static PsstRuleRegistry* GetInstance();

  // Returns the matched PSST rule, if any.
  void CheckIfMatch(
      const GURL& url,
      base::OnceCallback<void(const std::optional<MatchedRule>&)> cb);

  void LoadRules(const base::FilePath& path);

 private:
  PsstRuleRegistry();
  friend base::NoDestructor<PsstRuleRegistry>;

  friend class PsstTabHelperBrowserTest;
  friend class PsstRuleRegistryUnitTest;
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest, RulesLoading);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest,
                           CheckIfMatchWithNoRulesLoaded);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest,
                           RulesLoadingBrokenRulesFile);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest, RulesLoadingEmptyPath);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest,
                           RulesLoadingNonExistingPath);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest, LoadConcreteRule);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest, DoNotMatchRuleIfNotExists);
  FRIEND_TEST_ALL_PREFIXES(PsstRuleRegistryUnitTest,
                           RuleReferencesToNotExistedPath);

  void OnLoadRules(const std::string& data);
  void SetOnLoadCallbackForTest(
      base::OnceCallback<void(const std::string&, const std::vector<PsstRule>&)>
          callback);
  std::optional<base::OnceCallback<void(const std::string&,
                                        const std::vector<PsstRule>&)>>
      onload_test_callback_;

  std::vector<PsstRule> rules_;
  base::FilePath component_path_;
  base::WeakPtrFactory<PsstRuleRegistry> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_REGISTRY_H_

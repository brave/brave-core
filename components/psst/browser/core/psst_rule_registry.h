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
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace psst {

class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstRuleRegistry {
 public:
  PsstRuleRegistry(const PsstRuleRegistry&) = delete;
  PsstRuleRegistry& operator=(const PsstRuleRegistry&) = delete;

  virtual ~PsstRuleRegistry() = default;

  // Returns the matched PSST rule, if any.
  virtual void CheckIfMatch(
      const GURL& url,
      base::OnceCallback<void(const std::optional<MatchedRule>&)> cb) const = 0;

  // Given a path to psst.json, loads the rules from the file into memory.
  virtual void LoadRules(const base::FilePath& path) = 0;

 protected:
  PsstRuleRegistry() = default;

 private:
  virtual void OnLoadRules(const std::string& data) = 0;

  // Needed for testing private methods.
  friend class PsstTabHelperBrowserTest;
  friend class PsstTabHelperUnitTest;
  friend class PsstRuleRegistryUnitTest;
};

class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstRuleRegistryAccessor {
 public:
  PsstRuleRegistryAccessor();
  PsstRuleRegistryAccessor(const PsstRuleRegistryAccessor&) = delete;
  PsstRuleRegistryAccessor& operator=(const PsstRuleRegistryAccessor&) = delete;
  ~PsstRuleRegistryAccessor();

  static PsstRuleRegistryAccessor* GetInstance();  // singleton

  PsstRuleRegistry* Registry();

 private:
  friend class PsstTabHelperBrowserTest;
  friend class PsstRuleRegistryUnitTest;

  void SetRegistryForTesting(std::unique_ptr<PsstRuleRegistry> new_inst);

  std::unique_ptr<PsstRuleRegistry> impl_;
};

// This class loads and stores the rules from the psst.json file.
// It is also used for matching based on the URL.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstRuleRegistryImpl
    : public PsstRuleRegistry {
 public:
  PsstRuleRegistryImpl();
  PsstRuleRegistryImpl(const PsstRuleRegistryImpl&) = delete;
  PsstRuleRegistryImpl& operator=(const PsstRuleRegistryImpl&) = delete;
  ~PsstRuleRegistryImpl() override;
  void CheckIfMatch(const GURL& url,
                    base::OnceCallback<void(const std::optional<MatchedRule>&)>
                        cb) const override;
  // Given a path to psst.json, loads the rules from the file into memory.
  void LoadRules(const base::FilePath& path) override;

 protected:
  // These methods are also called by PsstTabHelperBrowserTest.
  // Given contents of psst.json, loads the rules from the file into memory.
  // Called by |LoadRules| after the file is read.
  void OnLoadRules(const std::string& data) override;

 private:
  void SetRuleDataReaderForTest(
      std::unique_ptr<RuleDataReader> rule_data_reader);

  std::unique_ptr<RuleDataReader> rule_data_reader_;
  std::vector<PsstRule> rules_;

  base::WeakPtrFactory<PsstRuleRegistryImpl> weak_factory_{this};

  // Needed for testing private methods.
  friend class PsstTabHelperBrowserTest;
  friend class PsstTabHelperUnitTest;
  friend struct base::DefaultSingletonTraits<PsstRuleRegistry>;
  friend class PsstRuleRegistryUnitTest;
  friend class MockPsstRuleRegistryImpl;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_REGISTRY_H_

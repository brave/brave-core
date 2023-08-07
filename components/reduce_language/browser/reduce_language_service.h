/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REDUCE_LANGUAGE_BROWSER_REDUCE_LANGUAGE_SERVICE_H_
#define BRAVE_COMPONENTS_REDUCE_LANGUAGE_BROWSER_REDUCE_LANGUAGE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/reduce_language/browser/reduce_language_component_installer.h"
#include "brave/components/reduce_language/browser/reduce_language_rule.h"
#include "brave/components/reduce_language/browser/reduce_language_service.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class PrefRegistrySimple;

namespace reduce_language {

// Manage Reduce-Language ruleset and provide an API for navigation throttles to
// call to determine if a URL is included in the ruleset.
class ReduceLanguageService
    : public KeyedService,
      public ReduceLanguageComponentInstallerPolicy::Observer {
 public:
  ReduceLanguageService();
  ReduceLanguageService(const ReduceLanguageService&) = delete;
  ReduceLanguageService& operator=(const ReduceLanguageService&) = delete;
  ~ReduceLanguageService() override;
  void OnRulesReady(const std::string&) override;

  bool ShouldReduceLanguage(const GURL& url) const;

 private:
  std::vector<std::unique_ptr<ReduceLanguageRule>> rules_;
  base::flat_set<std::string> excluded_host_cache_;

  base::WeakPtrFactory<ReduceLanguageService> weak_factory_{this};
};

}  // namespace reduce_language

#endif  // BRAVE_COMPONENTS_REDUCE_LANGUAGE_BROWSER_REDUCE_LANGUAGE_SERVICE_H_

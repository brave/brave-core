/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/reduce_language/browser/reduce_language_service.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "brave/components/reduce_language/browser/reduce_language_component_installer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

namespace reduce_language {

ReduceLanguageService::ReduceLanguageService() {}

ReduceLanguageService::~ReduceLanguageService() = default;

void ReduceLanguageService::OnRulesReady(const std::string& json_content) {
  auto parsed_rules = ReduceLanguageRule::ParseRules(json_content);
  if (!parsed_rules.has_value()) {
    DVLOG(1) << "Error: no rules parsed. " << parsed_rules.error();
    return;
  }
  rules_.clear();
  excluded_host_cache_.clear();
  rules_ = std::move(parsed_rules.value().first);
  excluded_host_cache_ = parsed_rules.value().second;
  DVLOG(1) << excluded_host_cache_.size() << " unique hosts, " << rules_.size()
           << " rules parsed from " << kReduceLanguageConfigFile;
}

bool ReduceLanguageService::ShouldReduceLanguage(const GURL& url) const {
  // Check host cache
  const std::string etldp1 =
      ReduceLanguageRule::GetETLDForReduceLanguage(url.host());
  if (base::Contains(excluded_host_cache_, etldp1)) {
    return false;
  }

  for (const std::unique_ptr<ReduceLanguageRule>& rule : rules_) {
    if (rule->AppliesTo(url)) {
      return false;
    }
  }

  return true;
}

}  // namespace reduce_language

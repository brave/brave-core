/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_service.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "brave/components/request_otr/browser/request_otr_component_installer.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

namespace request_otr {

RequestOTRService::RequestOTRService() {}

RequestOTRService::~RequestOTRService() = default;

void RequestOTRService::OnRulesReady(const std::string& json_content) {
  auto parsed_rules = RequestOTRRule::ParseRules(json_content);
  if (!parsed_rules.has_value()) {
    DVLOG(1) << "Error: no rules parsed. " << parsed_rules.error();
    return;
  }
  rules_.clear();
  host_cache_.clear();
  rules_ = std::move(parsed_rules.value().first);
  host_cache_ = parsed_rules.value().second;
  DVLOG(1) << host_cache_.size() << " unique hosts, " << rules_.size()
           << " rules parsed from " << kRequestOTRConfigFile;
}

bool RequestOTRService::ShouldBlock(const GURL& url) const {
  // Check host cache
  const std::string etldp1 = RequestOTRRule::GetETLDForRequestOTR(url.host());
  if (!base::Contains(host_cache_, etldp1)) {
    return false;
  }

  for (const std::unique_ptr<RequestOTRRule>& rule : rules_) {
    if (rule->ShouldBlock(url)) {
      return true;
    }
  }

  return false;
}

// static
void RequestOTRService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kRequestOTRActionOption,
                                static_cast<int>(RequestOTRActionOption::kAsk));
}

}  // namespace request_otr

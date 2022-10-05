/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_service.h"

#include <memory>
#include <string>
#include <vector>

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

RequestOTRService::RequestOTRService(
    RequestOTRComponentInstaller* component_installer)
    : component_installer_(component_installer) {}

RequestOTRService::~RequestOTRService() = default;

bool RequestOTRService::ShouldBlock(const GURL& url) const {
  // Check host cache
  const std::string etldp1 = RequestOTRRule::GetETLDForRequestOTR(url.host());
  const base::flat_set<std::string>& host_cache =
      component_installer_->host_cache();
  if (!base::Contains(host_cache, etldp1)) {
    return false;
  }

  const std::vector<std::unique_ptr<RequestOTRRule>>& rules =
      component_installer_->rules();
  for (const std::unique_ptr<RequestOTRRule>& rule : rules) {
    if (rule->ShouldBlock(url)) {
      return true;
    }
  }

  return false;
}

// static
void RequestOTRService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kRequestOTRActionOption,
                                static_cast<int>(RequestOTRActionOption::kAsk));
}

}  // namespace request_otr

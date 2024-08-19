// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/core/browser/debounce_service.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "brave/components/debounce/core/browser/debounce_component_installer.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace debounce {

DebounceService::DebounceService(
    DebounceComponentInstaller* component_installer,
    PrefService* prefs)
    : component_installer_(component_installer), prefs_(prefs) {}

DebounceService::~DebounceService() = default;

bool DebounceService::Debounce(const GURL& original_url,
                               GURL* final_url) const {
  // Check host cache to see if this URL needs to have any debounce rules
  // applied.
  const base::flat_set<std::string>& host_cache =
      component_installer_->host_cache();
  const std::string etldp1 =
      DebounceRule::GetETLDForDebounce(original_url.host());
  if (!base::Contains(host_cache, etldp1)) {
    return false;
  }

  const std::vector<std::unique_ptr<DebounceRule>>& rules =
      component_installer_->rules();

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    if (rule->Apply(original_url, final_url, prefs_)) {
      if (original_url != *final_url) {
        return true;
      }
    }
  }
  return false;
}

// static
void DebounceService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kDebounceEnabled, true);  // default on
}

bool DebounceService::IsEnabled() {
  return prefs_->GetBoolean(prefs::kDebounceEnabled);
}

void DebounceService::SetIsEnabled(const bool isEnabled) {
  prefs_->SetBoolean(prefs::kDebounceEnabled, isEnabled);
  prefs_->CommitPendingWrite();
}

}  // namespace debounce

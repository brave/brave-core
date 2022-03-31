/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/debounce/browser/debounce_service.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "brave/components/debounce/browser/debounce_component_installer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

namespace debounce {

DebounceService::DebounceService(
    DebounceComponentInstaller* component_installer)
    : component_installer_(component_installer) {}

DebounceService::~DebounceService() {}

bool DebounceService::Debounce(const GURL& original_url,
                               GURL* final_url) const {
  // Check host cache to see if this URL needs to have any debounce rules
  // applied.
  const base::flat_set<std::string>& host_cache =
      component_installer_->host_cache();
  const std::string etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          original_url, net::registry_controlled_domains::
                            PrivateRegistryFilter::INCLUDE_PRIVATE_REGISTRIES);
  if (!base::Contains(host_cache, etldp1))
    return false;

  bool changed = false;
  GURL current_url = original_url;
  const std::vector<std::unique_ptr<DebounceRule>>& rules =
      component_installer_->rules();

  // Debounce rules are applied in order. All rules are checked on every URL. If
  // one rule applies, the URL is changed to the debounced URL and we continue
  // to apply the rest of the rules to the new URL. Previously checked rules are
  // not reapplied; i.e. we never restart the loop.
  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    if (rule->Apply(current_url, final_url)) {
      if (current_url != *final_url) {
        changed = true;
        current_url = *final_url;
      }
    }
  }
  return changed;
}

}  // namespace debounce

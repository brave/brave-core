/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/unstoppable_domains_service.h"

#include <utility>

#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/pref_names.h"
#include "brave/components/unstoppable_domains/unstoppable_domains_service_delegate.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace unstoppable_domains {

UnstoppableDomainsService::UnstoppableDomainsService(
    std::unique_ptr<UnstoppableDomainsServiceDelegate> delegate,
    content::BrowserContext* context,
    PrefService* local_state)
    : local_state_(local_state), delegate_(std::move(delegate)) {
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(local_state);
  pref_change_registrar_->Add(
      kResolveMethod,
      base::Bind(&UnstoppableDomainsService::OnPreferenceChanged,
                 base::Unretained(this)));
}

UnstoppableDomainsService::~UnstoppableDomainsService() = default;

// static
void UnstoppableDomainsService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
}

void UnstoppableDomainsService::OnPreferenceChanged() {
  delegate_->UpdateNetworkService();
}

bool UnstoppableDomainsService::IsResolveMethodAsk() {
  return local_state_->GetInteger(kResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

}  // namespace unstoppable_domains

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/decentralized_dns_service.h"

#include <utility>

#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/decentralized_dns_service_delegate.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace decentralized_dns {

DecentralizedDnsService::DecentralizedDnsService(
    std::unique_ptr<DecentralizedDnsServiceDelegate> delegate,
    content::BrowserContext* context,
    PrefService* local_state)
    : delegate_(std::move(delegate)) {
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(local_state);
  pref_change_registrar_->Add(
      kUnstoppableDomainsResolveMethod,
      base::BindRepeating(&DecentralizedDnsService::OnPreferenceChanged,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      kENSResolveMethod,
      base::BindRepeating(&DecentralizedDnsService::OnPreferenceChanged,
                          base::Unretained(this)));
}

DecentralizedDnsService::~DecentralizedDnsService() = default;

// static
void DecentralizedDnsService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kUnstoppableDomainsResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
  registry->RegisterIntegerPref(kENSResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
}

void DecentralizedDnsService::OnPreferenceChanged() {
  delegate_->UpdateNetworkService();
}

}  // namespace decentralized_dns

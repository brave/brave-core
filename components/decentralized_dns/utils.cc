/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/utils.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/features.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/net/decentralized_dns/constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace decentralized_dns {

namespace {

base::Value MakeSelectValue(ResolveMethodTypes value,
                            const std::u16string& name) {
  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey("value", base::Value(static_cast<int>(value)));
  item.SetKey("name", base::Value(name));
  return item;
}

}  // namespace

bool IsDecentralizedDnsEnabled() {
  return base::FeatureList::IsEnabled(features::kDecentralizedDns);
}

bool IsUnstoppableDomainsTLD(const GURL& url) {
  return base::EndsWith(url.host_piece(), kCryptoDomain);
}

bool IsUnstoppableDomainsResolveMethodAsk(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsUnstoppableDomainsResolveMethodDoH(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS);
}

bool IsUnstoppableDomainsResolveMethodEthereum(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ETHEREUM);
}

bool IsENSTLD(const GURL& url) {
  return base::EndsWith(url.host_piece(), kEthDomain);
}

bool IsENSResolveMethodAsk(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kENSResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsENSResolveMethodDoH(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kENSResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS);
}

base::Value GetResolveMethodList(Provider provider) {
  base::Value list(base::Value::Type::LIST);
  list.Append(MakeSelectValue(
      ResolveMethodTypes::ASK,
      l10n_util::GetStringUTF16(IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_ASK)));
  list.Append(
      MakeSelectValue(ResolveMethodTypes::DISABLED,
                      l10n_util::GetStringUTF16(
                          IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_DISABLED)));
  list.Append(MakeSelectValue(
      ResolveMethodTypes::DNS_OVER_HTTPS,
      l10n_util::GetStringUTF16(
          IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_DNS_OVER_HTTPS)));

  if (provider == Provider::UNSTOPPABLE_DOMAINS) {
    list.Append(
        MakeSelectValue(ResolveMethodTypes::ETHEREUM,
                        l10n_util::GetStringUTF16(
                            IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_ETHEREUM)));
  }

  return list;
}

}  // namespace decentralized_dns

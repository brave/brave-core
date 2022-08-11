/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/utils.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/net/decentralized_dns/constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
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

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kUnstoppableDomainsResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
  registry->RegisterIntegerPref(kENSResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 05/2022
  if (static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS) ==
      local_state->GetInteger(
          decentralized_dns::kUnstoppableDomainsResolveMethod)) {
    local_state->ClearPref(decentralized_dns::kUnstoppableDomainsResolveMethod);
  }
  if (static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS) ==
      local_state->GetInteger(decentralized_dns::kENSResolveMethod)) {
    local_state->ClearPref(decentralized_dns::kENSResolveMethod);
  }
}

bool IsUnstoppableDomainsTLD(const GURL& url) {
  if (!url.is_valid())
    return false;
  for (auto* domain : kUnstoppableDomains) {
    if (base::EndsWith(url.host_piece(), domain))
      return true;
  }
  return false;
}

bool IsUnstoppableDomainsResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsUnstoppableDomainsResolveMethodEthereum(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ETHEREUM);
}

bool IsENSTLD(const GURL& url) {
  if (!url.is_valid())
    return false;
  return base::EndsWith(url.host_piece(), kEthDomain);
}

bool IsENSResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kENSResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsENSResolveMethodEthereum(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kENSResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ETHEREUM);
}

base::Value GetResolveMethodList() {
  base::Value list(base::Value::Type::LIST);
  list.Append(MakeSelectValue(ResolveMethodTypes::ASK,
                              brave_l10n::GetLocalizedResourceUTF16String(
                                  IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_ASK)));
  list.Append(
      MakeSelectValue(ResolveMethodTypes::DISABLED,
                      brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_DISABLED)));
  list.Append(
      MakeSelectValue(ResolveMethodTypes::ETHEREUM,
                      brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_ETHEREUM)));

  return list;
}

}  // namespace decentralized_dns

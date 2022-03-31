/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/buildflags/buildflags.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#include "base/strings/strcat.h"
#include "brave/components/decentralized_dns/utils.h"
#include "brave/net/decentralized_dns/constants.h"
#include "net/dns/public/dns_over_https_config.h"
#endif

namespace {

// Add DoH servers to support decentralized DNS such as Unstoppable Domains and
// ENS. These servers are controlled using its own prefs and not
// kDnsOverHttpsTemplates pref as the servers we added here are special and
// only applies to certain TLD which is different from user's global DoH
// provider settings.
void AddDoHServers(net::DnsOverHttpsConfig* doh_config,
                   PrefService* local_state,
                   bool force_check_parental_controls_for_automatic_mode) {
  // force_check_parental_controls_for_automatic_mode is only true for
  // settings UI which we specifically do not want to display those special
  // resolvers we added.
  if (force_check_parental_controls_for_automatic_mode)
    return;

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
  std::string doh_config_string = doh_config->ToString();

  if (decentralized_dns::IsUnstoppableDomainsResolveMethodDoH(local_state) &&
      doh_config_string.find(
          decentralized_dns::kUnstoppableDomainsDoHResolver) ==
          std::string::npos) {
    doh_config_string =
        base::StrCat({decentralized_dns::kUnstoppableDomainsDoHResolver, " ",
                      doh_config_string});
  }

  if (decentralized_dns::IsENSResolveMethodDoH(local_state) &&
      doh_config_string.find(decentralized_dns::kENSDoHResolver) ==
          std::string::npos) {
    doh_config_string = base::StrCat(
        {decentralized_dns::kENSDoHResolver, " ", doh_config_string});
  }

  *doh_config = net::DnsOverHttpsConfig::FromStringLax(doh_config_string);
#endif
}

}  // namespace

#define BRAVE_GET_AND_UPDATE_CONFIGURATION \
  AddDoHServers(&doh_config, local_state_, \
                force_check_parental_controls_for_automatic_mode);

#include "src/chrome/browser/net/stub_resolver_config_reader.cc"
#undef BRAVE_GET_AND_UPDATE_CONFIGURATION

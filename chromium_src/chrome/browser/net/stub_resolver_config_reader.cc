/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/buildflags/buildflags.h"
#include "chrome/browser/net/secure_dns_util.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
#include "base/strings/strcat.h"
#include "brave/components/unstoppable_domains/utils.h"
#include "brave/net/unstoppable_domains/constants.h"
#endif

namespace chrome_browser_net {
namespace secure_dns {

// Add DoH servers to support Unstoppable Domains (and ENS in the future PR).
// These servers are controlled using its own prefs and not
// kDnsOverHttpsTemplates pref as the servers we added here are special and
// only applies to certain TLD which is different from user's global DoH
// provider settings.
std::vector<base::StringPiece> SplitGroup(
    std::string& group,
    PrefService* local_state,
    bool force_check_parental_controls_for_automatic_mode) {
#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
  // force_check_parental_controls_for_automatic_mode is only true for
  // settings UI which we specifically do not want to display those special
  // resolvers we added.
  if (!force_check_parental_controls_for_automatic_mode &&
      unstoppable_domains::IsResolveMethodDoH(local_state) &&
      group.find(unstoppable_domains::kDoHResolver) == std::string::npos) {
    group = base::StrCat({unstoppable_domains::kDoHResolver, " ", group});
  }
#endif

  return chrome_browser_net::secure_dns::SplitGroup(group);
}

}  // namespace secure_dns
}  // namespace chrome_browser_net

#define SplitGroup(doh_templates)         \
  SplitGroup(doh_templates, local_state_, \
             force_check_parental_controls_for_automatic_mode)
#include "../../../../../chrome/browser/net/stub_resolver_config_reader.cc"

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"
#include "brave/components/unstoppable_domains/buildflags/buildflags.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/utils.h"
#endif

namespace {

#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
void AddUnstoppableDomainsResolver(std::string* doh_templates,
                                   PrefService* local_state) {
  if (unstoppable_domains::IsResolveMethodDoH(local_state) &&
      doh_templates->find(unstoppable_domains::kDoHResolver) ==
          std::string::npos) {
    *doh_templates =
        base::StrCat({unstoppable_domains::kDoHResolver, " ", *doh_templates});
  }
}
#endif  // BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)

void AddDoHServers(std::string* doh_templates, PrefService* local_state) {
#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
  AddUnstoppableDomainsResolver(doh_templates, local_state);
#endif
}

}  // namespace

#define BRAVE_GET_AND_UPDATE_CONFIGURATION \
  AddDoHServers(&doh_templates, local_state_);

#include "../../../../../chrome/browser/net/stub_resolver_config_reader.cc"
#undef BRAVE_GET_AND_UPDATE_CONFIGURATION

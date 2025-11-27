// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "extensions/buildflags/buildflags.h"
#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/extension_urls.h"
#endif

namespace {

bool IsChromeWebStoreURL(const GURL& url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return (url.GetHost() == extension_urls::GetWebstoreLaunchURL().GetHost()) ||
         (url.GetHost() == extension_urls::GetNewWebstoreLaunchURL().GetHost());
#else
  return false;
#endif
}

}  // namespace

// Add Brave-specific restrictions
#define BRAVE_MAY_ACT_ON_URL_INTERNAL                       \
  if (IsChromeWebStoreURL(url)) {                           \
    decision_wrapper->Reject(                               \
        "Extension store URL",                              \
        actor::MayActOnUrlBlockReason::kUrlNotInAllowlist); \
    return;                                                 \
  }

#include <chrome/browser/actor/site_policy.cc>
#undef BRAVE_MAY_ACT_ON_URL_INTERNAL

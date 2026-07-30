// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "extensions/buildflags/buildflags.h"
#include "url/gurl.h"
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

// Add Brave-specific restrictions before delegating to the origin gating
// checker. Applied at both the page-action (`SafetyChecksForNextAction`) and
// navigation (`IsAcceptableNavigationDestination`) entry points for `url`.
#define BRAVE_MAY_ACT_ON_URL_INTERNAL(callback)                             \
  if (IsChromeWebStoreURL(url)) {                                           \
    journal_->Log(                                                          \
        url, task_->id(), "OriginGatingDecision",                           \
        JournalDetailsBuilder().AddError("Extension store URL").Build());   \
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(               \
        FROM_HERE,                                                          \
        base::BindOnce(callback,                                            \
                       actor::MayActOnUrlBlockReason::kUrlNotInAllowlist)); \
    return;                                                                 \
  }

#include <chrome/browser/actor/execution_engine.cc>
#undef BRAVE_MAY_ACT_ON_URL_INTERNAL

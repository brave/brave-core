/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_tab_util.h"

#include <string>

#include "base/feature_list.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/cookie_store_id.h"
#include "brave/components/containers/core/common/features.h"
#endif

namespace {

// Matches chrome.cookies store ids: "0", "1", or "containers:<uuid>".
std::string BraveGetCookieStoreIdForTab(content::WebContents* contents) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return containers::GetStoreIdForWebContents(contents);
  }
#endif
  return contents->GetBrowserContext()->IsOffTheRecord() ? "1" : "0";
}

}  // namespace

#include <chrome/browser/extensions/extension_tab_util.cc>

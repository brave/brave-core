// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/favicon/favicon_utils.h"

#include <optional>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/url_constants.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#endif

namespace favicon {

std::optional<bool> ShouldThemifyFaviconForEntryBraveImpl(
    content::NavigationEntry* entry);

}  // namespace favicon

#include <chrome/browser/favicon/favicon_utils.cc>

namespace favicon {

std::optional<bool> ShouldThemifyFaviconForEntryBraveImpl(
    content::NavigationEntry* entry) {
  const GURL& virtual_url = entry->GetVirtualURL();
  // Don't theme for certain brave favicons which are full color.
  if (virtual_url.SchemeIs(content::kChromeUIScheme) &&
      (virtual_url.host() == kRewardsPageHost ||
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
       virtual_url.host() == kWalletPageHost ||
#endif
       virtual_url.host() == kAIChatUIHost ||
       virtual_url.host() == chrome::kChromeUINewTabHost)) {
    return false;
  }
  return std::nullopt;
}

}  // namespace favicon

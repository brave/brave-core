/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/prefs/brave_pref_service_incognito_allowlist.h"

#include <array>

#include "base/containers/span.h"
#include "base/strings/cstring_view.h"
#include "brave/browser/ui/bookmark/brave_bookmark_prefs.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/browser/pref_names.h"
#endif

namespace brave {

base::span<const base::cstring_view> GetBravePersistentPrefNames() {
  static constexpr auto kAllowlist = std::to_array<base::cstring_view>({
      kBraveAutofillPrivateWindows,
#if !BUILDFLAG(IS_ANDROID)
      kShowWalletIconOnToolbar,
      prefs::kSidePanelHorizontalAlignment,
      kTabMuteIndicatorNotClickable,
      brave_tabs::kVerticalTabsExpandedWidth,
      brave_tabs::kVerticalTabsEnabled,
      brave_tabs::kVerticalTabsCollapsed,
      brave_tabs::kVerticalTabsFloatingEnabled,
      brave_tabs::kVerticalTabsShowTitleOnWindow,
      brave_tabs::kVerticalTabsOnRight,
      brave_tabs::kVerticalTabsShowScrollbar,
      brave_tabs::kSharedPinnedTab,
#endif
#if defined(TOOLKIT_VIEWS)
      sidebar::kSidePanelWidth,
#endif
      ai_chat::prefs::kLastAcceptedDisclaimer,
      ai_chat::prefs::kBraveChatAutocompleteProviderEnabled,
      brave::bookmarks::prefs::kShowAllBookmarksButton,
  });

  return kAllowlist;
}

}  // namespace brave

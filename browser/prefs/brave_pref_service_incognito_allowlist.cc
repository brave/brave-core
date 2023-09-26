/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/prefs/brave_pref_service_incognito_allowlist.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/pref_names.h"
#endif

namespace brave {

const std::vector<const char*>& GetBravePersistentPrefNames() {
  static base::NoDestructor<std::vector<const char*>> brave_allowlist({
    kBraveAutofillPrivateWindows,
#if !BUILDFLAG(IS_ANDROID)
        prefs::kSidePanelHorizontalAlignment, kTabMuteIndicatorNotClickable,
        brave_tabs::kVerticalTabsExpandedWidth,
        brave_tabs::kVerticalTabsEnabled, brave_tabs::kVerticalTabsCollapsed,
        brave_tabs::kVerticalTabsFloatingEnabled,
#endif
#if defined(TOOLKIT_VIEWS)
        sidebar::kSidePanelWidth,
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
        ai_chat::prefs::kBraveChatHasSeenDisclaimer,
        ai_chat::prefs::kBraveChatAutoGenerateQuestions,
        ai_chat::prefs::kBraveChatAutocompleteProviderEnabled,
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
  });

  return *brave_allowlist;
}

}  // namespace brave

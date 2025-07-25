// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/ai_chat_profile.h"

#include "brave/browser/ai_chat/ai_chat_profile.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"

namespace ai_chat {

namespace {

const SkColor kAIChatAgentProfileThemeColor = SkColorSetRGB(253, 58, 122);
const char16_t kAIChatAgentProfileName[] = u"Leo AI Content Agent";

void SetupAndOpenAIChatAgentProfile(Profile* profile) {
  // This runs for every profile open so that we can update
  // with any changes to the profile.

  // Set theme
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
  theme_service->SetUserColor(kAIChatAgentProfileThemeColor);
  // Assume user has opted-in in the owning profile in order to get
  // here, so we can copy that preference.
  ai_chat::SetUserOptedIn(profile->GetPrefs(), true);
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  ProfileAttributesEntry* attributes =
      storage.GetProfileAttributesWithPath(GetAIChatAgentProfileDir());
  attributes->SetIsOmitted(true);
  attributes->SetLocalProfileName(kAIChatAgentProfileName, false);

  // Open browser window
  profiles::OpenBrowserWindowForProfile(
      base::BindOnce([](Browser* browser) {
        // Open sidebar
        browser->GetFeatures().side_panel_ui()->Show(SidePanelEntryId::kChatUI);
      }),
      false, false, false, profile);
}

}  // namespace

void OpenBrowserWindowForAIChatAgentProfile() {
  if (!features::IsAIChatAgenticProfileEnabled()) {
    return;
  }
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  // We don't provide a profile-init callback because we want to ensure
  // the prefs are up to date each time.
  profile_manager->CreateProfileAsync(
      GetAIChatAgentProfileDir(),
      base::BindOnce(&SetupAndOpenAIChatAgentProfile));
}

}  // namespace ai_chat

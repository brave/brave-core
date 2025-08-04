// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"

#include "base/path_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_paths.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#endif

namespace ai_chat {

namespace {

#if !BUILDFLAG(IS_ANDROID)
const SkColor kAIChatAgentProfileThemeColor = SkColorSetRGB(253, 58, 122);
#endif

const char16_t kAIChatAgentProfileName[] = u"Leo AI Content Agent";

void SetupAndOpenAIChatAgentProfile(base::OnceCallback<void(Browser*)> callback,
                                    Profile* profile) {
  // This runs for every profile open so that we can update
  // with any changes to the profile.

  // Assume user has opted-in in some profile already in order to get
  // here, so we can copy that preference.
  ai_chat::SetUserOptedIn(profile->GetPrefs(), true);

  // Set profile name
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  ProfileAttributesEntry* attributes =
      storage.GetProfileAttributesWithPath(profile->GetPath());
  attributes->SetLocalProfileName(kAIChatAgentProfileName, false);

#if !BUILDFLAG(IS_ANDROID)
  // Set theme
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
  theme_service->SetUserColor(kAIChatAgentProfileThemeColor);

  // Open browser window
  profiles::OpenBrowserWindowForProfile(
      base::BindOnce(
          [](base::OnceCallback<void(Browser*)> callback, Browser* browser) {
            // Open sidebar
            browser->GetFeatures().side_panel_ui()->Show(
                SidePanelEntryId::kChatUI);
            std::move(callback).Run(browser);
          },
          std::move(callback)),
      false, false, false, profile);
#endif
}

void OpenBrowserWindowForAIChatAgentProfileWithCallback(
    Profile* from_profile,
    base::OnceCallback<void(Browser*)> callback) {
  CHECK(from_profile);
  CHECK(IsAIChatEnabled(from_profile->GetPrefs()));
  CHECK(!from_profile->IsAIChatAgent());
  // This should not be called if the feature is disabled
  if (!features::IsAIChatAgenticProfileEnabled()) {
    DLOG(ERROR) << __func__ << " AI Chat Agentic Profile feature is disabled";
    std::move(callback).Run(nullptr);
    return;
  }
  // This should not be callable if the current profile has not yet opted-in to
  // AI Chat.
  if (!HasUserOptedIn(from_profile->GetPrefs())) {
    DLOG(ERROR) << __func__ << " Existing profile has not opted-in to AI Chat";
    std::move(callback).Run(nullptr);
    return;
  }

  // We don't provide a profile-init callback because we want to ensure
  // the prefs are up to date each time.
  base::FilePath profile_path =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  profile_path = profile_path.Append(brave::kAIChatAgentProfileDir);

  g_browser_process->profile_manager()->CreateProfileAsync(
      profile_path,
      base::BindOnce(&SetupAndOpenAIChatAgentProfile, std::move(callback)));
}

}  // namespace

void OpenBrowserWindowForAIChatAgentProfile(Profile* from_profile) {
  OpenBrowserWindowForAIChatAgentProfileWithCallback(from_profile,
                                                     base::DoNothing());
}

#if !BUILDFLAG(IS_ANDROID)
void OpenBrowserWindowForAIChatAgentProfileForTesting(
    Profile* from_profile,
    base::OnceCallback<void(Browser*)> callback) {
  OpenBrowserWindowForAIChatAgentProfileWithCallback(from_profile,
                                                     std::move(callback));
}
#endif

}  // namespace ai_chat

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"

#include "base/path_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_paths.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/profile_id.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#endif

static_assert(BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE));

namespace ai_chat {

namespace {

#if !BUILDFLAG(IS_ANDROID)
void OpenBrowserWindowAndSidePanel(base::OnceCallback<void(Browser*)> callback,
                                   Profile* profile) {
  if (!profile) {
    DLOG(ERROR) << "Could not create profile";
    std::move(callback).Run(nullptr);
    return;
  }

  // Open browser window
  profiles::OpenBrowserWindowForProfile(
      base::BindOnce(
          [](base::OnceCallback<void(Browser*)> callback, Browser* browser) {
            // Open sidebar when a browser window first opens
            // TODO(petemill): Move this to the AIChatAgentProfileManager
            // on `BrowserListObserver::OnBrowserAdded` when the kChatUI side
            // panel is global and not per-tab.
            SidePanelUI* side_panel_ui = browser->GetFeatures().side_panel_ui();
            if (side_panel_ui) {
              side_panel_ui->Show(SidePanelEntryId::kChatUI);
            }
            std::move(callback).Run(browser);
          },
          std::move(callback)),
      /*always_create=*/false, /*is_new_profile=*/false,
      /*open_command_line_urls=*/false, profile);
}

void OpenBrowserWindowForAIChatAgentProfileWithCallback(
    Profile& from_profile,
    base::OnceCallback<void(Browser*)> callback) {
  CHECK(IsAIChatEnabled(from_profile.GetPrefs()));
  CHECK(!from_profile.IsAIChatAgent());
  // This should not be called if the feature is disabled
  if (!features::IsAIChatAgentProfileEnabled()) {
    DLOG(ERROR) << __func__ << " AI Chat Agent Profile feature is disabled";
    std::move(callback).Run(nullptr);
    return;
  }

  // We don't provide a profile-init callback because we want to ensure
  // the prefs are up to date each time.
  // TODO(https://github.com/brave/brave-browser/issues/48188): Don't use
  // a harcoded path for the profile, use an attribute instead.
  base::FilePath profile_path =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  profile_path = profile_path.Append(brave::kAIChatAgentProfileDir);

  // Brave Origin manages AI Chat as a profile-scoped policy that defaults to
  // disabled for newly-created profiles. The agent profile is created on behalf
  // of a user whose source profile has AI Chat enabled (guaranteed by the CHECK
  // above), so copy the source profile's managed value onto the soon-to-be
  // created agent profile. Without this the agent profile would inherit Brave
  // Origin's default-disabled value, which leaves AI Chat - and therefore the
  // agent profile itself - non-functional (and previously crashed when its
  // kChatUI side panel was shown for an unregistered entry). This is written
  // before CreateProfileAsync() so the value is already present when the agent
  // profile's policies are first loaded. When Brave Origin is not in use this
  // is a no-op and the agent profile keeps the unmanaged default (enabled).
  if (brave_origin::IsBraveOriginPurchased()) {
    brave_origin::BraveOriginPolicyManager::GetInstance()->SetPolicyValue(
        policy::key::kBraveAIChatEnabled,
        from_profile.GetPrefs()->GetBoolean(prefs::kEnabledByPolicy),
        brave_origin::GetProfileId(profile_path));
  }

  g_browser_process->profile_manager()->CreateProfileAsync(
      profile_path,
      base::BindOnce(&OpenBrowserWindowAndSidePanel, std::move(callback)));
}
#endif  // !BUILDFLAG(IS_ANDROID)
}  // namespace

void OpenBrowserWindowForAIChatAgentProfile(Profile& from_profile) {
#if BUILDFLAG(IS_ANDROID)
  NOTREACHED();
#else
  OpenBrowserWindowForAIChatAgentProfileWithCallback(from_profile,
                                                     base::DoNothing());
#endif
}

#if !BUILDFLAG(IS_ANDROID)
void OpenBrowserWindowForAIChatAgentProfileForTesting(
    Profile& from_profile,
    base::OnceCallback<void(Browser*)> callback) {
  OpenBrowserWindowForAIChatAgentProfileWithCallback(from_profile,
                                                     std::move(callback));
}
#endif

}  // namespace ai_chat

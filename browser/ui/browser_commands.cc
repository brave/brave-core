/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include <string>

#include "base/files/file_path.h"
#include "brave/app/brave_command_ids.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/profile_picker.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/speedreader_service.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_profile_service.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#endif

using content::WebContents;

namespace {
}  // namespace

namespace brave {

void NewOffTheRecordWindowTor(Browser* browser) {
  if (browser->profile()->IsTor()) {
    chrome::OpenEmptyWindow(browser->profile());
    return;
  }

  TorProfileManager::SwitchToTorProfile(browser->profile(), base::DoNothing());
}

void NewTorConnectionForSite(Browser* browser) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->profile();
  DCHECK(profile);
  tor::TorProfileService* service =
    TorProfileServiceFactory::GetForContext(profile);
  DCHECK(service);
  WebContents* current_tab =
    browser->tab_strip_model()->GetActiveWebContents();
  if (!current_tab)
    return;
  service->SetNewTorCircuit(current_tab);
#endif
}

void AddNewProfile() {
  ProfilePicker::Show(ProfilePicker::EntryPoint::kProfileMenuAddNewProfile);
}

void OpenGuestProfile() {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  DCHECK(service->GetBoolean(prefs::kBrowserGuestModeEnabled));
  profiles::SwitchToGuestProfile(base::DoNothing());
}

void MaybeDistillAndShowSpeedreaderBubble(Browser* browser) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  using DistillState = speedreader::DistillState;
  WebContents* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (contents) {
    auto* tab_helper =
        speedreader::SpeedreaderTabHelper::FromWebContents(contents);
    if (!tab_helper)
      return;

    const DistillState state = tab_helper->PageDistillState();
    switch (state) {
      case DistillState::kSpeedreaderMode:
      case DistillState::kSpeedreaderOnDisabledPage:
        tab_helper->ShowSpeedreaderBubble();
        break;
      case DistillState::kReaderMode:
        // Refresh the page (toggles off Speedreader)
        contents->GetController().Reload(content::ReloadType::NORMAL, false);
        break;
      case DistillState::kPageProbablyReadable:
        tab_helper->SingleShotSpeedreader();
        break;
      default:
        NOTREACHED();
    }
  }
#endif  // BUILDFLAG(ENABLE_SPEEDREADER)
}

void ShowBraveVPNBubble(Browser* browser) {
  // Ask to browser view.
  static_cast<BraveBrowserWindow*>(browser->window())->ShowBraveVPNBubble();
}

void ToggleBraveVPNButton(Browser* browser) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* prefs = browser->profile()->GetPrefs();
  const bool show = prefs->GetBoolean(brave_vpn::prefs::kBraveVPNShowButton);
  prefs->SetBoolean(brave_vpn::prefs::kBraveVPNShowButton, !show);
#endif
}

void OpenBraveVPNUrls(Browser* browser, int command_id) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  std::string target_url;
  switch (command_id) {
    case IDC_SEND_BRAVE_VPN_FEEDBACK:
      target_url = brave_vpn::kFeedbackUrl;
      break;
    case IDC_ABOUT_BRAVE_VPN:
      target_url = brave_vpn::kAboutUrl;
      break;
    case IDC_MANAGE_BRAVE_VPN_PLAN:
      target_url = brave_vpn::GetManageUrl();
      break;
    default:
      NOTREACHED();
  }

  chrome::AddTabAt(browser, GURL(target_url), -1, true);
#endif
}

void ShowWalletBubble(Browser* browser) {
#if defined(TOOLKIT_VIEWS)
  static_cast<BraveBrowserView*>(browser->window())->CreateWalletBubble();
#endif
}

void ShowApproveWalletBubble(Browser* browser) {
#if defined(TOOLKIT_VIEWS)
  static_cast<BraveBrowserView*>(browser->window())
      ->CreateApproveWalletBubble();
#endif
}

void CloseWalletBubble(Browser* browser) {
#if defined(TOOLKIT_VIEWS)
  static_cast<BraveBrowserView*>(browser->window())->CloseWalletBubble();
#endif
}

}  // namespace brave

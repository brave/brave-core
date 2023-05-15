/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include <string>

#include "base/files/file_path.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/debounce/debounce_service_factory.h"
#include "brave/browser/net/brave_query_filter.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/debounce/browser/debounce_service.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/profile_picker.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/clipboard/clipboard_buffer.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

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
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/common/channel_info.h"
#endif

using content::WebContents;

namespace {
}  // namespace

namespace brave {

void NewOffTheRecordWindowTor(Browser* browser) {
  CHECK(browser);
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
  ProfilePicker::Show(ProfilePicker::Params::FromEntryPoint(
      ProfilePicker::EntryPoint::kProfileMenuAddNewProfile));
}

void OpenGuestProfile() {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  DCHECK(service->GetBoolean(prefs::kBrowserGuestModeEnabled));
  profiles::SwitchToGuestProfile(base::DoNothing());
}

void MaybeDistillAndShowSpeedreaderBubble(Browser* browser) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  WebContents* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents)
    return;
  if (auto* tab_helper =
          speedreader::SpeedreaderTabHelper::FromWebContents(contents)) {
    tab_helper->ProcessIconClick();
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

void OpenIpfsFilesWebUI(Browser* browser) {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  auto* prefs = browser->profile()->GetPrefs();
  DCHECK(ipfs::IsLocalGatewayConfigured(prefs));
  GURL gateway = ipfs::GetAPIServer(chrome::GetChannel());
  GURL::Replacements replacements;
  replacements.SetPathStr("/webui/");
  replacements.SetRefStr("/files");
  auto target_url = gateway.ReplaceComponents(replacements);
  chrome::AddTabAt(browser, GURL(target_url), -1, true);
#endif
}

void OpenBraveVPNUrls(Browser* browser, int command_id) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVpnService* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(browser->profile());
  std::string target_url;
  switch (command_id) {
    case IDC_SEND_BRAVE_VPN_FEEDBACK:
      target_url = brave_vpn::kFeedbackUrl;
      break;
    case IDC_ABOUT_BRAVE_VPN:
      target_url = brave_vpn::kAboutUrl;
      break;
    case IDC_MANAGE_BRAVE_VPN_PLAN:
      target_url =
          brave_vpn::GetManageUrl(vpn_service->GetCurrentEnvironment());
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

void CopySanitizedURL(Browser* browser, const GURL& url) {
  GURL sanitized_url = brave::URLSanitizerServiceFactory::GetForBrowserContext(
                           browser->profile())
                           ->SanitizeURL(url);

  ui::ScopedClipboardWriter scw(ui::ClipboardBuffer::kCopyPaste);
  scw.WriteText(base::UTF8ToUTF16(sanitized_url.spec()));
}

// Copies an url cleared through:
// - Debouncer (potentially debouncing many levels)
// - Query filter
// - URLSanitizerService
void CopyLinkWithStrictCleaning(Browser* browser, const GURL& url) {
  DCHECK(url.SchemeIsHTTPOrHTTPS());
  GURL final_url;
  // Apply debounce rules.
  auto* debounce_service =
      debounce::DebounceServiceFactory::GetForBrowserContext(
          browser->profile());
  if (debounce_service && !debounce_service->Debounce(url, &final_url)) {
    VLOG(1) << "Unable to apply debounce rules";
    final_url = url;
  }
  // Apply query filters.
  auto filtered_url = ApplyQueryFilter(final_url);
  if (filtered_url.has_value()) {
    final_url = filtered_url.value();
  }
  // Sanitize url.
  final_url = brave::URLSanitizerServiceFactory::GetForBrowserContext(
                  browser->profile())
                  ->SanitizeURL(final_url);

  ui::ScopedClipboardWriter scw(ui::ClipboardBuffer::kCopyPaste);
  scw.WriteText(base::UTF8ToUTF16(final_url.spec()));
}

void ToggleWindowTitleVisibilityForVerticalTabs(Browser* browser) {
  auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();
  prefs->SetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow,
      !prefs->GetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow));
}

void ToggleVerticalTabStrip(Browser* browser) {
  auto* profile = browser->profile()->GetOriginalProfile();
  auto* prefs = profile->GetPrefs();
  const bool was_using_vertical_tab_strip =
      prefs->GetBoolean(brave_tabs::kVerticalTabsEnabled);
  prefs->SetBoolean(brave_tabs::kVerticalTabsEnabled,
                    !was_using_vertical_tab_strip);
}

void ToggleVerticalTabStripFloatingMode(Browser* browser) {
  auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();
  prefs->SetBoolean(
      brave_tabs::kVerticalTabsFloatingEnabled,
      !prefs->GetBoolean(brave_tabs::kVerticalTabsFloatingEnabled));
}

void ToggleActiveTabAudioMute(Browser* browser) {
  WebContents* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents || !contents->IsCurrentlyAudible())
    return;

  bool mute_tab = !contents->IsAudioMuted();
  chrome::SetTabAudioMuted(contents, mute_tab, TabMutedReason::AUDIO_INDICATOR,
                           std::string());
}

void ToggleSidebarPosition(Browser* browser) {
  auto* prefs = browser->profile()->GetPrefs();
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment,
                    !prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
}

void ToggleSidebar(Browser* browser) {
  if (!browser) {
    return;
  }

  if (auto* brave_browser_window =
          BraveBrowserWindow::From(browser->window())) {
    brave_browser_window->ToggleSidebar();
  }
}

bool HasSelectedURL(Browser* browser) {
  if (!browser) {
    return false;
  }
  auto* brave_browser_window = BraveBrowserWindow::From(browser->window());
  return brave_browser_window && brave_browser_window->HasSelectedURL();
}

void CleanAndCopySelectedURL(Browser* browser) {
  if (!browser) {
    return;
  }
  auto* brave_browser_window = BraveBrowserWindow::From(browser->window());
  if (brave_browser_window) {
    brave_browser_window->CleanAndCopySelectedURL();
  }
}
}  // namespace brave

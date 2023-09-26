/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include <numeric>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/debounce/debounce_service_factory.h"
#include "brave/browser/ui/brave_shields_data_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/query_filter/utils.h"
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
#include "chrome/browser/ui/profiles/profile_picker.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/common/pref_names.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/clipboard/clipboard_buffer.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "url/origin.h"

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

#if BUILDFLAG(IS_WIN)
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/common/channel_info.h"
#endif

using content::WebContents;

namespace brave {
void NewOffTheRecordWindowTor(Browser* browser) {
  CHECK(browser);
  if (browser->profile()->IsTor()) {
    chrome::OpenEmptyWindow(browser->profile());
    return;
  }

  TorProfileManager::SwitchToTorProfile(browser->profile());
}

void NewTorConnectionForSite(Browser* browser) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->profile();
  DCHECK(profile);
  tor::TorProfileService* service =
      TorProfileServiceFactory::GetForContext(profile);
  DCHECK(service);
  WebContents* current_tab = browser->tab_strip_model()->GetActiveWebContents();
  if (!current_tab) {
    return;
  }
  service->SetNewTorCircuit(current_tab);
#endif
}

void MaybeDistillAndShowSpeedreaderBubble(Browser* browser) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  WebContents* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents) {
    return;
  }
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

void ToggleBraveVPNTrayIcon() {
#if BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)
  brave_vpn::EnableVPNTrayIcon(!brave_vpn::IsVPNTrayIconEnabled());
  if (brave_vpn::IsVPNTrayIconEnabled()) {
    brave_vpn::wireguard::ShowBraveVpnStatusTrayIcon();
  }
#endif
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
  CHECK(vpn_service);
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
  if (!browser || !browser->profile()) {
    return;
  }
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
  if (!browser || !browser->profile()) {
    return;
  }
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
  auto filtered_url = query_filter::ApplyQueryFilter(final_url);
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

void ToggleVerticalTabStripExpanded(Browser* browser) {
  auto* prefs = browser->profile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed,
                    !prefs->GetBoolean(brave_tabs::kVerticalTabsCollapsed));
}

void ToggleActiveTabAudioMute(Browser* browser) {
  WebContents* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents || !contents->IsCurrentlyAudible()) {
    return;
  }

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

void ToggleShieldsEnabled(Browser* browser) {
  if (!browser) {
    return;
  }

  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents) {
    return;
  }
  auto* shields =
      brave_shields::BraveShieldsDataController::FromWebContents(contents);
  if (!shields) {
    return;
  }

  shields->SetBraveShieldsEnabled(!shields->GetBraveShieldsEnabled());
}

void ToggleJavascriptEnabled(Browser* browser) {
  if (!browser) {
    return;
  }

  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents) {
    return;
  }
  auto* shields =
      brave_shields::BraveShieldsDataController::FromWebContents(contents);
  if (!shields) {
    return;
  }

  shields->SetIsNoScriptEnabled(!shields->GetNoScriptEnabled());
}

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
void ShowPlaylistBubble(Browser* browser) {
  BraveBrowserWindow::From(browser->window())->ShowPlaylistBubble();
}
#endif

void GroupTabsOnCurrentOrigin(Browser* browser) {
  auto url =
      browser->tab_strip_model()->GetActiveWebContents()->GetVisibleURL();
  auto origin = url::Origin::Create(url);

  std::vector<int> group_indices;
  for (int index = 0; index < browser->tab_strip_model()->count(); ++index) {
    auto* tab = browser->tab_strip_model()->GetWebContentsAt(index);
    auto tab_origin = url::Origin::Create(tab->GetVisibleURL());
    if (origin.IsSameOriginWith(tab_origin)) {
      group_indices.push_back(index);
    }
  }
  auto group_id = browser->tab_strip_model()->AddToNewGroup(group_indices);
  auto* group =
      browser->tab_strip_model()->group_model()->GetTabGroup(group_id);

  auto data = *group->visual_data();
  data.SetTitle(base::UTF8ToUTF16(origin.host()));
  group->SetVisualData(data);
}

void MoveGroupToNewWindow(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  auto current_group_id = tsm->GetTabGroupForTab(tsm->active_index());
  if (!current_group_id.has_value()) {
    return;
  }

  tsm->delegate()->MoveGroupToNewWindow(current_group_id.value());
}

void CloseDuplicateTabs(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  auto url = tsm->GetActiveWebContents()->GetVisibleURL();

  for (int i = tsm->GetTabCount() - 1; i >= 0; --i) {
    // Don't close the active tab.
    if (tsm->active_index() == i) {
      continue;
    }

    auto* tab = tsm->GetWebContentsAt(i);
    if (tab->GetVisibleURL() == url) {
      tab->Close();
    }
  }
}

bool CanCloseTabsToLeft(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  const auto& selection = tsm->selection_model();
  if (selection.empty()) {
    return false;
  }

  int left_selected = *(selection.selected_indices().begin());
  return left_selected > 0;
}

void CloseTabsToLeft(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  const auto& selection = tsm->selection_model();
  if (selection.empty()) {
    return;
  }

  int left_selected = *(selection.selected_indices().begin());
  for (int i = left_selected - 1; i >= 0; --i) {
    tsm->CloseWebContentsAt(i, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB |
                                   TabCloseTypes::CLOSE_USER_GESTURE);
  }
}

bool CanCloseUnpinnedTabs(Browser* browser) {
  auto first_unpinned_index =
      browser->tab_strip_model()->IndexOfFirstNonPinnedTab();
  return first_unpinned_index < browser->tab_strip_model()->count();
}

void CloseUnpinnedTabs(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  DCHECK(CanCloseUnpinnedTabs(browser));

  for (int i = tsm->count() - 1; i >= tsm->IndexOfFirstNonPinnedTab(); --i) {
    tsm->CloseWebContentsAt(i, TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB |
                                   TabCloseTypes::CLOSE_USER_GESTURE);
  }
}

void AddAllTabsToNewGroup(Browser* browser) {
  std::vector<int> indices(browser->tab_strip_model()->count());
  std::iota(indices.begin(), indices.end(), 0);
  browser->tab_strip_model()->AddToNewGroup(indices);
}

bool CanMuteAllTabs(Browser* browser, bool exclude_active) {
  auto* tsm = browser->tab_strip_model();
  for (int i = 0; i < tsm->count(); ++i) {
    if (exclude_active && tsm->active_index() == i) {
      continue;
    }

    auto* contents = tsm->GetWebContentsAt(i);
    if (contents->IsCurrentlyAudible()) {
      return true;
    }
  }
  return false;
}

void MuteAllTabs(Browser* browser, bool exclude_active) {
  auto* tsm = browser->tab_strip_model();
  for (int i = 0; i < tsm->count(); ++i) {
    if (exclude_active && tsm->active_index() == i) {
      continue;
    }

    auto* contents = tsm->GetWebContentsAt(i);
    if (contents->IsCurrentlyAudible()) {
      contents->SetAudioMuted(true);
    }
  }
}

bool CanUnmuteAllTabs(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  for (int i = 0; i < tsm->count(); ++i) {
    auto* contents = tsm->GetWebContentsAt(i);
    if (contents->IsAudioMuted()) {
      return true;
    }
  }
  return false;
}

void UnmuteAllTabs(Browser* browser) {
  auto* tsm = browser->tab_strip_model();
  for (int i = 0; i < tsm->count(); ++i) {
    auto* contents = tsm->GetWebContentsAt(i);
    if (contents->IsAudioMuted()) {
      contents->SetAudioMuted(false);
    }
  }
}

void ScrollTabToTop(Browser* browser) {
  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  contents->ScrollToTopOfDocument();
}

void ScrollTabToBottom(Browser* browser) {
  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  contents->ScrollToBottomOfDocument();
}

}  // namespace brave

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/education_page_delegate_desktop.h"

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"

namespace brave_education {

EducationPageDelegateDesktop::EducationPageDelegateDesktop(
    tabs::TabInterface& tab)
    : tab_(tab) {}

EducationPageDelegateDesktop::~EducationPageDelegateDesktop() = default;

void EducationPageDelegateDesktop::OpenURL(const GURL& url,
                                           WindowOpenDisposition disposition) {
  tab_->GetBrowserWindowInterface()->OpenGURL(url, disposition);
}

void EducationPageDelegateDesktop::OpenRewardsPanel() {
  // TODO(https://github.com/brave/brave-browser/issues/42179): Instead of using
  // a `Browser` pointer, expose Rewards panel functionality via
  // `BrowserWindowFeatures`.
  auto* browser =
      tab_->GetBrowserWindowInterface()->GetBrowserForMigrationOnly();
  CHECK(browser);
  if (auto* panel_coordinator =
          brave_rewards::RewardsPanelCoordinator::FromBrowser(browser)) {
    panel_coordinator->OpenRewardsPanel();
  }
}

void EducationPageDelegateDesktop::OpenVPNPanel() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  tab_->GetBrowserWindowInterface()
      ->GetFeatures()
      .GetBraveVPNController()
      ->ShowBraveVPNBubble(/* show_select */ false);
#endif
}

void EducationPageDelegateDesktop::OpenAIChat() {
  tab_->GetBrowserWindowInterface()->GetFeatures().side_panel_ui()->Show(
      SidePanelEntry::Key(SidePanelEntryId::kChatUI));
}

}  // namespace brave_education

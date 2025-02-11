/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/brave_education_page_delegate_desktop.h"

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"

namespace brave_education {

BraveEducationPageDelegateDesktop::BraveEducationPageDelegateDesktop(
    BrowserWindowInterface& window_interface)
    : window_interface_(window_interface) {}

BraveEducationPageDelegateDesktop::~BraveEducationPageDelegateDesktop() =
    default;

void BraveEducationPageDelegateDesktop::OpenURL(
    const GURL& url,
    WindowOpenDisposition disposition) {
  window_interface_->OpenGURL(url, disposition);
}

void BraveEducationPageDelegateDesktop::OpenRewardsPanel() {
  // TODO(zenparsing): Instead of using a `Browser` pointer,
  // expose Rewards panel functionality via `BrowserWindowFeatures`.
  // See https://github.com/brave/brave-browser/issues/42179.
  auto* browser = window_interface_->GetBrowserForMigrationOnly();
  CHECK(browser);
  if (auto* panel_coordinator =
          brave_rewards::RewardsPanelCoordinator::FromBrowser(browser)) {
    panel_coordinator->OpenRewardsPanel();
  }
}

void BraveEducationPageDelegateDesktop::OpenVPNPanel() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  window_interface_->GetFeatures().GetBraveVPNController()->ShowBraveVPNBubble(
      /* show_select */ false);
#endif
}

void BraveEducationPageDelegateDesktop::OpenAIChat() {
  window_interface_->GetFeatures().side_panel_ui()->Show(
      SidePanelEntry::Key(SidePanelEntryId::kChatUI));
}

}  // namespace brave_education

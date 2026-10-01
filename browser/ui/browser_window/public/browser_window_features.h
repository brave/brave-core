/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"

class AIChatSidePanelTabTransferBridge;
class BraveShieldsUIContentsCache;
class BraveNonClientHitTestHelper;
class BraveVPNController;
class FocusModeController;
class PlaylistSidePanelCoordinator;
class TreeTabSessionManager;
class VerticalTabController;
class WorkspacesBubbleController;

namespace brave_rewards {
class RewardsPanelCoordinator;
}  // namespace brave_rewards

namespace sidebar {
class SidebarController;
}  // namespace sidebar

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
namespace email_aliases {
class EmailAliasesController;
}  // namespace email_aliases
#endif

namespace screenshot {
class ScreenshotController;
}  // namespace screenshot

// This file doesn't include header file for BrowserWindowFeatures_ChromiumImpl
// because this file only could be included at the bottom of
// //chrome/browser/ui/browser_window/public/browser_window_features.h. So we
// could avoid dependency cycle with //chrome/browser/ui/browser_window.
class BrowserWindowFeatures : public BrowserWindowFeatures_ChromiumImpl {
 public:
  BrowserWindowFeatures();
  ~BrowserWindowFeatures() override;

  // BrowserWindowFeatures_ChromiumImpl:
  void Init(BrowserWindowInterface* browser) override;
  void InitPostBrowserViewConstruction(BrowserView* browser_view) override;
  void TearDownPreBrowserWindowDestruction() override;

  FocusModeController* focus_mode_controller() {
    return focus_mode_controller_.get();
  }

  const FocusModeController* focus_mode_controller() const {
    return focus_mode_controller_.get();
  }

 private:
  std::unique_ptr<sidebar::SidebarController> sidebar_controller_;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  std::unique_ptr<BraveVPNController> brave_vpn_controller_;
#endif
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  std::unique_ptr<brave_rewards::RewardsPanelCoordinator>
      rewards_panel_coordinator_;
#endif
#if BUILDFLAG(ENABLE_PLAYLIST)
  std::unique_ptr<PlaylistSidePanelCoordinator>
      playlist_side_panel_coordinator_;
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<AIChatSidePanelTabTransferBridge>
      ai_chat_side_panel_tab_transfer_bridge_;
#endif
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  std::unique_ptr<email_aliases::EmailAliasesController>
      email_aliases_controller_;
#endif
  std::unique_ptr<FocusModeController> focus_mode_controller_;
  std::unique_ptr<BraveShieldsUIContentsCache> brave_shields_ui_contents_cache_;
  std::unique_ptr<BraveNonClientHitTestHelper>
      brave_non_client_hit_test_helper_;
  std::unique_ptr<TreeTabSessionManager> tree_tab_session_manager_;
  std::unique_ptr<screenshot::ScreenshotController> screenshot_controller_;
  std::unique_ptr<VerticalTabController> vertical_tab_controller_;
  std::unique_ptr<WorkspacesBubbleController> workspaces_bubble_controller_;
};

#endif  // BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

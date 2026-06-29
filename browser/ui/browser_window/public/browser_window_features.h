/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"

class AIChatSidePanelTabTransferBridge;
class GURL;
class BraveShieldsUIContentsCache;
class BraveNonClientHitTestHelper;
class BraveVPNController;
class FocusModeController;
class PlaylistSidePanelCoordinator;
class TreeTabSessionManager;
class VerticalTabController;
class WalletSidePanelCoordinator;
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

  sidebar::SidebarController* sidebar_controller() {
    return sidebar_controller_.get();
  }

  brave_rewards::RewardsPanelCoordinator* rewards_panel_coordinator();

  BraveVPNController* brave_vpn_controller();

#if BUILDFLAG(ENABLE_PLAYLIST)
  PlaylistSidePanelCoordinator* playlist_side_panel_coordinator() {
    return playlist_side_panel_coordinator_.get();
  }
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  // Null unless the `kAIChatMoveFullPageToSidePanel` feature is enabled and
  // this is a normal window with AI Chat available.
  AIChatSidePanelTabTransferBridge* ai_chat_side_panel_tab_transfer_bridge() {
    return ai_chat_side_panel_tab_transfer_bridge_.get();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  WalletSidePanelCoordinator* wallet_side_panel_coordinator() {
    return wallet_side_panel_coordinator_.get();
  }

  // Routes |url| to the wallet side panel when it is showing. Lives on
  // BrowserWindowFeatures to avoid a GN dependency cycle between tab_helper and
  // the wallet side panel coordinator.
  bool NavigateWalletSidePanelIfActive(const GURL& url);
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  email_aliases::EmailAliasesController* email_aliases_controller() {
    return email_aliases_controller_.get();
  }
#endif

  FocusModeController* focus_mode_controller() {
    return focus_mode_controller_.get();
  }

  const FocusModeController* focus_mode_controller() const {
    return focus_mode_controller_.get();
  }

  BraveShieldsUIContentsCache* brave_shields_ui_contents_cache() {
    return brave_shields_ui_contents_cache_.get();
  }

  BraveNonClientHitTestHelper* brave_non_client_hit_test_helper() {
    return brave_non_client_hit_test_helper_.get();
  }

  // Can be null when the browser isn't a normal browser or when the tree tab
  // feature is disabled.
  TreeTabSessionManager* GetTreeTabSessionManager() {
    return tree_tab_session_manager_.get();
  }

  screenshot::ScreenshotController* screenshot_controller() {
    return screenshot_controller_.get();
  }

  VerticalTabController* vertical_tab_controller() {
    return vertical_tab_controller_.get();
  }

  const VerticalTabController* vertical_tab_controller() const {
    return vertical_tab_controller_.get();
  }

  WorkspacesBubbleController* workspaces_bubble_controller() {
    return workspaces_bubble_controller_.get();
  }

  void SetVerticalTabControllerForTesting(
      std::unique_ptr<VerticalTabController> vertical_tab_controller);

 private:
  std::unique_ptr<sidebar::SidebarController> sidebar_controller_;
  std::unique_ptr<BraveVPNController> brave_vpn_controller_;
  std::unique_ptr<brave_rewards::RewardsPanelCoordinator>
      rewards_panel_coordinator_;
#if BUILDFLAG(ENABLE_PLAYLIST)
  std::unique_ptr<PlaylistSidePanelCoordinator>
      playlist_side_panel_coordinator_;
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<AIChatSidePanelTabTransferBridge>
      ai_chat_side_panel_tab_transfer_bridge_;
#endif
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  std::unique_ptr<WalletSidePanelCoordinator> wallet_side_panel_coordinator_;
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

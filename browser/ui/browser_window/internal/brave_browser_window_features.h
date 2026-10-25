/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_WINDOW_INTERNAL_BRAVE_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_BROWSER_UI_BROWSER_WINDOW_INTERNAL_BRAVE_BROWSER_WINDOW_FEATURES_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "ui/base/unowned_user_data/user_data_factory.h"

class AIChatSidePanelTabTransferBridge;
class BraveNonClientHitTestHelper;
class BraveShieldsUIContentsCache;
class BraveVPNController;
class BrowserView;
class BrowserWindowInterface;
class PeekController;
class PlaylistSidePanelCoordinator;
class TreeTabSessionManager;
class WindowClosingConfirmController;
class WorkspacesBubbleController;

namespace brave_rewards {
class RewardsPanelCoordinator;
}  // namespace brave_rewards

namespace email_aliases {
class EmailAliasesController;
}  // namespace email_aliases

namespace screenshot {
class ScreenshotController;
}  // namespace screenshot

namespace sidebar {
class SidebarController;
}  // namespace sidebar

// An extension of BrowserWindowFeatures that includes all the features that
// are specific to Brave's browser window.
class BraveBrowserWindowFeatures {
 public:
  explicit BraveBrowserWindowFeatures(BrowserWindowInterface& browser);
  BraveBrowserWindowFeatures(const BraveBrowserWindowFeatures&) = delete;
  BraveBrowserWindowFeatures& operator=(const BraveBrowserWindowFeatures&) =
      delete;
  ~BraveBrowserWindowFeatures();

  void Init();
  void InitPostBrowserViewConstruction(BrowserView* browser_view);
  void TearDownPreBrowserWindowDestruction();

 private:
  // Features are created through BrowserWindowFeatures' factory so tests can
  // override them.
  static ui::UserDataFactoryWithOwner<BrowserWindowInterface>&
  GetUserDataFactory();

  const raw_ref<BrowserWindowInterface> browser_;

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

  std::unique_ptr<BraveShieldsUIContentsCache> brave_shields_ui_contents_cache_;

  std::unique_ptr<BraveNonClientHitTestHelper>
      brave_non_client_hit_test_helper_;

  std::unique_ptr<TreeTabSessionManager> tree_tab_session_manager_;

  std::unique_ptr<screenshot::ScreenshotController> screenshot_controller_;

  std::unique_ptr<WorkspacesBubbleController> workspaces_bubble_controller_;

  std::unique_ptr<PeekController> peek_controller_;

  std::unique_ptr<WindowClosingConfirmController>
      window_closing_confirm_controller_;
};

#endif  // BRAVE_BROWSER_UI_BROWSER_WINDOW_INTERNAL_BRAVE_BROWSER_WINDOW_FEATURES_H_

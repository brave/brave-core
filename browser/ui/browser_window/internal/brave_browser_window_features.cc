/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_window/internal/brave_browser_window_features.h"

#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/types/to_address.h"
#include "base/unguessable_token.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/screenshot/screenshot_controller.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/public/before_unload_ignore_controller.h"
#include "brave/browser/ui/tabs/public/closing_last_tab_controller.h"
#include "brave/browser/ui/tabs/public/tab_cycling_controller.h"
#include "brave/browser/ui/tabs/tree_tab_session_manager.h"
#include "brave/browser/ui/views/brave_help_bubble/shields_help_bubble_controller.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/page_info/brave_shields_ui_contents_cache.h"
#include "brave/browser/ui/views/toolbar/screenshot_preview_dialog.h"
#include "brave/browser/ui/views/workspaces/workspaces_bubble_controller.h"
#include "brave/browser/ui/window_closing_confirm/window_closing_confirm_controller.h"
#include "brave/browser/workspaces/features.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_group_sync/tab_group_sync_service_factory.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "printing/buildflags/buildflags.h"
#include "ui/gfx/native_ui_types.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "brave/browser/screenshot/print_preview_extractor_factory.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/ui/email_aliases/email_aliases_controller.h"
#include "brave/components/email_aliases/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/playlist/core/browser/utils.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.h"
#include "brave/components/ai_chat/core/common/features.h"
#endif

BraveBrowserWindowFeatures::BraveBrowserWindowFeatures(
    BrowserWindowInterface& browser)
    : browser_(browser) {}

BraveBrowserWindowFeatures::~BraveBrowserWindowFeatures() = default;

// static
ui::UserDataFactoryWithOwner<BrowserWindowInterface>&
BraveBrowserWindowFeatures::GetUserDataFactory() {
  return BrowserWindowFeatures::GetUserDataFactory();
}

void BraveBrowserWindowFeatures::Init() {
  auto* profile = browser_->GetProfile();

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  if (brave_rewards::RewardsServiceFactory::GetForProfile(profile)) {
    rewards_panel_coordinator_ =
        GetUserDataFactory()
            .CreateInstance<brave_rewards::RewardsPanelCoordinator>(
                *browser_, base::to_address(browser_));
  }
#endif

  brave_shields_ui_contents_cache_ =
      GetUserDataFactory().CreateInstance<BraveShieldsUIContentsCache>(
          *browser_, browser_->GetUnownedUserDataHost());

  window_closing_confirm_controller_ =
      GetUserDataFactory().CreateInstance<WindowClosingConfirmController>(
          *browser_, *browser_);

  closing_last_tab_controller_ =
      GetUserDataFactory().CreateInstance<ClosingLastTabController>(*browser_,
                                                                    *browser_);

  before_unload_ignore_controller_ =
      GetUserDataFactory().CreateInstance<BeforeUnloadIgnoreController>(
          *browser_, *browser_);

  brave_non_client_hit_test_helper_ =
      GetUserDataFactory().CreateInstance<BraveNonClientHitTestHelper>(
          *browser_, browser_->GetUnownedUserDataHost());

  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab) &&
      browser_->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL) {
    tree_tab_session_manager_ =
        GetUserDataFactory().CreateInstance<TreeTabSessionManager>(
            *browser_, browser_->GetUnownedUserDataHost(), profile,
            browser_->GetTabStripModel(), browser_->GetSessionID());
  }

  // Created before the window, as `BraveBrowserView` hands the sidebar its view
  // and the side panel registers the Playlist entry while they are built.
  if (sidebar::CanUseSidebar(base::to_address(browser_))) {
    sidebar_controller_ =
        GetUserDataFactory().CreateInstance<sidebar::SidebarController>(
            *browser_, base::to_address(browser_), profile);
#if BUILDFLAG(ENABLE_PLAYLIST)
    if (playlist::IsPlaylistAllowed(profile->GetPrefs())) {
      playlist_side_panel_coordinator_ =
          GetUserDataFactory().CreateInstance<PlaylistSidePanelCoordinator>(
              *browser_, base::to_address(browser_), profile);
    }
#endif  // BUILDFLAG(ENABLE_PLAYLIST)
  }
}

void BraveBrowserWindowFeatures::InitPostWindowConstruction() {
  // WebUI browser windows have none of the views UI these features drive.
  auto* brave_browser_window =
      BraveBrowserWindow::FromBrowser(base::to_address(browser_));
  if (!brave_browser_window) {
    return;
  }

  // The fullscreen controller only exists once the window is constructed.
  brave_browser_window->ReadyToListenFullscreenChanges();

  shields_help_bubble_controller_ =
      GetUserDataFactory().CreateInstance<ShieldsHelpBubbleController>(
          *browser_, *browser_);

  tab_cycling_controller_ =
      GetUserDataFactory().CreateInstance<TabCyclingController>(*browser_,
                                                                *browser_);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_ =
      GetUserDataFactory().CreateInstance<BraveVPNController>(*browser_,
                                                              *browser_);
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  if (email_aliases::features::IsEmailAliasesEnabled()) {
    if (auto* email_aliases_service =
            email_aliases::EmailAliasesServiceFactory::GetServiceForProfile(
                browser_->GetProfile())) {
      email_aliases_controller_ =
          GetUserDataFactory()
              .CreateInstance<email_aliases::EmailAliasesController>(
                  *browser_, *browser_, email_aliases_service);
    }
  }
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  {
    auto extractor =
        screenshot::CreatePrintPreviewExtractor(base::BindRepeating(
            []() -> base::flat_map<base::UnguessableToken, int>& {
              return printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap();
            }));
    screenshot_controller_ =
        GetUserDataFactory().CreateInstance<screenshot::ScreenshotController>(
            *browser_, browser_->GetUnownedUserDataHost(),
            browser_->GetProfile(),
            base::BindRepeating(
                [](BrowserWindowInterface* browser) -> gfx::NativeWindow {
                  return browser->GetWindow()->GetNativeWindow();
                },
                base::to_address(browser_)),
            base::BindRepeating(&screenshot::ShowScreenshotPreviewDialog),
            std::move(extractor));
  }
#else
  screenshot_controller_ =
      GetUserDataFactory().CreateInstance<screenshot::ScreenshotController>(
          *browser_, browser_->GetUnownedUserDataHost(), browser_->GetProfile(),
          base::BindRepeating(
              [](BrowserWindowInterface* browser) -> gfx::NativeWindow {
                return browser->GetWindow()->GetNativeWindow();
              },
              base::to_address(browser_)),
          base::BindRepeating(&screenshot::ShowScreenshotPreviewDialog));
#endif

  if (base::FeatureList::IsEnabled(features::kWorkspaces) &&
      browser_->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL) {
    workspaces_bubble_controller_ =
        GetUserDataFactory().CreateInstance<WorkspacesBubbleController>(
            *browser_, browser_->GetUnownedUserDataHost());
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (base::FeatureList::IsEnabled(
          ai_chat::features::kAIChatMoveFullPageToSidePanel) &&
      browser_->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL &&
      ai_chat::AIChatServiceFactory::GetForBrowserContext(
          browser_->GetProfile())) {
    ai_chat_side_panel_tab_transfer_bridge_ =
        GetUserDataFactory().CreateInstance<AIChatSidePanelTabTransferBridge>(
            *browser_, base::to_address(browser_));
  }
#endif
}

void BraveBrowserWindowFeatures::TearDownPreBrowserWindowDestruction() {
  shields_help_bubble_controller_.reset();
  tab_cycling_controller_.reset();
  screenshot_controller_.reset();
#if BUILDFLAG(ENABLE_AI_CHAT)
  ai_chat_side_panel_tab_transfer_bridge_.reset();
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_.reset();
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  email_aliases_controller_.reset();
#endif

  if (sidebar_controller_) {
    sidebar_controller_->TearDownPreBrowserWindowDestruction();
#if BUILDFLAG(ENABLE_PLAYLIST)
    playlist_side_panel_coordinator_.reset();
#endif
  }
}

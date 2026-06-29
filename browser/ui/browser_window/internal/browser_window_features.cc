/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "base/unguessable_token.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/screenshot/screenshot_controller.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/tabs/tree_tab_session_manager.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/page_info/brave_shields_ui_contents_cache.h"
#include "brave/browser/ui/views/workspaces/workspaces_bubble_controller.h"
#include "brave/browser/workspaces/features.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_coordinator.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#endif
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_group_sync/tab_group_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
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

#if !BUILDFLAG(ENABLE_BRAVE_REWARDS)
namespace brave_rewards {
class RewardsPanelCoordinator {};
}  // namespace brave_rewards
#endif

#if !BUILDFLAG(ENABLE_BRAVE_VPN)
// Use stub class to avoid incomplete type build error.
class BraveVPNController {};
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

BrowserWindowFeatures::BrowserWindowFeatures() = default;
BrowserWindowFeatures::~BrowserWindowFeatures() = default;

brave_rewards::RewardsPanelCoordinator*
BrowserWindowFeatures::rewards_panel_coordinator() {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  return rewards_panel_coordinator_.get();
#else
  NOTREACHED();
#endif
}

BraveVPNController* BrowserWindowFeatures::brave_vpn_controller() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  return brave_vpn_controller_.get();
#else
  NOTREACHED();
#endif
}

void BrowserWindowFeatures::Init(BrowserWindowInterface* browser) {
  BrowserWindowFeatures_ChromiumImpl::Init(browser);

  auto* profile = browser->GetProfile();

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  if (brave_rewards::RewardsServiceFactory::GetForProfile(profile)) {
    rewards_panel_coordinator_ =
        std::make_unique<brave_rewards::RewardsPanelCoordinator>(browser);
  }
#endif

  brave_shields_ui_contents_cache_ =
      std::make_unique<BraveShieldsUIContentsCache>();

  brave_non_client_hit_test_helper_ =
      std::make_unique<BraveNonClientHitTestHelper>();

  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab) &&
      browser->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL) {
    tree_tab_session_manager_ = std::make_unique<TreeTabSessionManager>(
        profile, browser->GetTabStripModel(), browser->GetSessionID());
  }

  if (BrowserSupportsFocusMode(browser)) {
    focus_mode_controller_ = std::make_unique<FocusModeController>();
  }

  // VerticalTabController should be constructed in Init() instead of
  // InitPostBrowserViewConstruction() because it would be referenced by many
  // views.
  vertical_tab_controller_ = std::make_unique<VerticalTabController>(
      browser->GetType(), profile->GetPrefs(), focus_mode_controller_.get());
}

void BrowserWindowFeatures::InitPostBrowserViewConstruction(
    BrowserView* browser_view) {
  if (sidebar::CanUseSidebar(browser_view->browser())) {
    sidebar_controller_ = std::make_unique<sidebar::SidebarController>(
        browser_view->browser(), browser_view->GetProfile());
#if BUILDFLAG(ENABLE_PLAYLIST)
    if (playlist::IsPlaylistAllowed(browser_view->GetProfile()->GetPrefs())) {
      playlist_side_panel_coordinator_ =
          std::make_unique<PlaylistSidePanelCoordinator>(
              browser_view->browser(), sidebar_controller_.get(),
              browser_view->GetProfile());
    }
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    if (browser_view->browser()->GetType() ==
            BrowserWindowInterface::Type::TYPE_NORMAL &&
        brave_wallet::IsAllowed(browser_view->GetProfile()->GetPrefs()) &&
        base::FeatureList::IsEnabled(
            brave_wallet::features::kBraveWalletSidePanel)) {
      wallet_side_panel_coordinator_ =
          std::make_unique<WalletSidePanelCoordinator>(browser_view->browser());
    }
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
  }

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  if (email_aliases::features::IsEmailAliasesEnabled()) {
    if (auto* email_aliases_service =
            email_aliases::EmailAliasesServiceFactory::GetServiceForProfile(
                browser_view->GetProfile())) {
      email_aliases_controller_ =
          std::make_unique<email_aliases::EmailAliasesController>(
              browser_view, email_aliases_service);
    }
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn_controller_ = std::make_unique<BraveVPNController>(browser_view);
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  {
    auto extractor =
        screenshot::CreatePrintPreviewExtractor(base::BindRepeating(
            []() -> base::flat_map<base::UnguessableToken, int>& {
              return printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap();
            }));
    screenshot_controller_ = std::make_unique<screenshot::ScreenshotController>(
        browser_view->GetProfile(),
        base::BindRepeating(
            [](BrowserView* bv) -> gfx::NativeWindow {
              return bv->GetNativeWindow();
            },
            browser_view),
        std::move(extractor));
  }
#else
  screenshot_controller_ = std::make_unique<screenshot::ScreenshotController>(
      browser_view->GetProfile(), base::BindRepeating(
                                      [](BrowserView* bv) -> gfx::NativeWindow {
                                        return bv->GetNativeWindow();
                                      },
                                      browser_view));
#endif

  if (base::FeatureList::IsEnabled(features::kWorkspaces) &&
      browser_->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL) {
    workspaces_bubble_controller_ =
        std::make_unique<WorkspacesBubbleController>();
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (base::FeatureList::IsEnabled(
          ai_chat::features::kAIChatMoveFullPageToSidePanel) &&
      browser_->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL &&
      ai_chat::AIChatServiceFactory::GetForBrowserContext(
          browser_->GetProfile())) {
    ai_chat_side_panel_tab_transfer_bridge_ =
        std::make_unique<AIChatSidePanelTabTransferBridge>(browser_);
  }
#endif

  BrowserWindowFeatures_ChromiumImpl::InitPostBrowserViewConstruction(
      browser_view);
}

void BrowserWindowFeatures::TearDownPreBrowserWindowDestruction() {
  BrowserWindowFeatures_ChromiumImpl::TearDownPreBrowserWindowDestruction();
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
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    wallet_side_panel_coordinator_.reset();
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
  }
}

void BrowserWindowFeatures::SetVerticalTabControllerForTesting(
    std::unique_ptr<VerticalTabController> vertical_tab_controller) {
  vertical_tab_controller_ = std::move(vertical_tab_controller);
}

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
bool BrowserWindowFeatures::NavigateWalletSidePanelIfActive(const GURL& url) {
  if (!wallet_side_panel_coordinator_) {
    return false;
  }

  auto* panel_ui = side_panel_ui();
  if (!panel_ui) {
    return false;
  }

  if (!panel_ui->IsSidePanelEntryShowing(
          SidePanelEntry::Key(SidePanelEntryId::kWallet))) {
    return false;
  }

  wallet_side_panel_coordinator_->Navigate(url);
  return true;
}
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

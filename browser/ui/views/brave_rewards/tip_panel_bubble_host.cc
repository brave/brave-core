/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_rewards/tip_panel_bubble_host.h"

#include <memory>
#include <optional>
#include <utility>

#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ui/webui/brave_rewards/tip_panel_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/grit/brave_components_strings.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

namespace brave_rewards {

namespace {

// `TipPanelBubbleManager` disables web contents caching for the tip panel.
class TipPanelBubbleManager : public WebUIBubbleManagerImpl<TipPanelUI> {
 public:
  TipPanelBubbleManager(views::View* anchor_view, Profile* profile)
      : WebUIBubbleManagerImpl(anchor_view,
                               profile,
                               GURL(kBraveTipPanelURL),
                               IDS_BRAVE_UI_BRAVE_REWARDS,
                               /*force_load_on_create=*/false) {}

  ~TipPanelBubbleManager() override = default;

  // WebUIBubbleManager:
  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog(
      const std::optional<gfx::Rect>& anchor,
      views::BubbleBorder::Arrow arrow) override {
    set_cached_contents_wrapper(nullptr);
    return WebUIBubbleManagerImpl::CreateWebUIBubbleDialog(anchor, arrow);
  }
};

views::View* GetAnchorView(Browser* browser) {
  DCHECK(browser);
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  DCHECK(browser_view);
  auto* location_bar_view = browser_view->GetLocationBarView();
  DCHECK(location_bar_view);
  return location_bar_view;
}

}  // namespace

TipPanelBubbleHost::TipPanelBubbleHost(Browser* browser)
    : BrowserUserData<TipPanelBubbleHost>(*browser) {
  auto* coordinator = TipPanelCoordinator::FromBrowser(browser);
  DCHECK(coordinator);
  coordinator->AddObserver(this);
}

TipPanelBubbleHost::~TipPanelBubbleHost() {
  if (auto* coordinator = TipPanelCoordinator::FromBrowser(&GetBrowser())) {
    coordinator->RemoveObserver(this);
  }
}

void TipPanelBubbleHost::MaybeCreateForBrowser(Browser* browser) {
  DCHECK(browser);
  if (IsSupportedForProfile(browser->profile())) {
    CreateForBrowser(browser);
  }
}

void TipPanelBubbleHost::OnTipPanelRequested(const std::string& publisher_id) {
  // No-op if the bubble is already open. If the bubble is already open and
  // showing a different publisher, then we can ignore this request and let the
  // user continue interacting with the already-open panel.
  if (bubble_manager_ && bubble_manager_->GetBubbleWidget()) {
    return;
  }

  // Create the bubble manager if necessary.
  if (!bubble_manager_) {
    bubble_manager_ = std::make_unique<TipPanelBubbleManager>(
        GetAnchorView(&GetBrowser()), GetBrowser().profile());
  }

  // Notify the panel coordinator of the browser size, so that it can size
  // itself appropriately.
  if (auto* coordinator = TipPanelCoordinator::FromBrowser(&GetBrowser())) {
    coordinator->set_browser_size(
        BrowserView::GetBrowserViewForBrowser(&GetBrowser())->size());
  }

  bubble_manager_->ShowBubble({}, views::BubbleBorder::TOP_CENTER);
}

BROWSER_USER_DATA_KEY_IMPL(TipPanelBubbleHost);

}  // namespace brave_rewards

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_rewards/tip_panel_bubble_host.h"

#include <memory>
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

// The tip dialog is anchored to the center of the location bar, and must be
// drawn with rounded bottom borders. We implement these customizations by
// subclassing `WebUIBubbleDialogView`. This implementation is then used by
// `TipPanelBubbleManager` when creating the dialog view.
class TipPanelDialogView : public WebUIBubbleDialogView {
 public:
  TipPanelDialogView(views::View* anchor_view,
                     BubbleContentsWrapper* contents_wrapper,
                     const absl::optional<gfx::Rect>& anchor_rect)
      : WebUIBubbleDialogView(anchor_view, contents_wrapper, anchor_rect) {
    SetArrow(views::BubbleBorder::TOP_CENTER);
  }

  // views::Widget:
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override {
    auto frame = WebUIBubbleDialogView::CreateNonClientFrameView(widget);
    auto* bubble_frame = static_cast<views::BubbleFrameView*>(frame.get());
    auto* bubble_border = bubble_frame->bubble_border();
    DCHECK(bubble_border);
    bubble_border->SetRoundedCorners(0, 0, 16, 16);
    return frame;
  }

  // views::View:
  void AddedToWidget() override {
    WebUIBubbleDialogView::AddedToWidget();
    web_view()->holder()->SetCornerRadii(gfx::RoundedCornersF(0, 0, 16, 16));
  }
};

// In order to use `TipPanelDialogView` as the dialog view implementation, we
// must override the `CreateWebUIBubbleDialog` of `WebUIBubbleManager`.
class TipPanelBubbleManager : public WebUIBubbleManager {
 public:
  TipPanelBubbleManager(views::View* anchor_view, Profile* profile)
      : anchor_view_(anchor_view), profile_(profile) {}

  ~TipPanelBubbleManager() override = default;

  // WebUIBubbleManager:

  // The persistent renderer feature is not supported for this bubble.
  void MaybeInitPersistentRenderer() override {}

  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog(
      const absl::optional<gfx::Rect>& anchor) override {
    auto contents_wrapper =
        std::make_unique<BubbleContentsWrapperT<TipPanelUI>>(
            GURL(kBraveTipPanelURL), profile_, IDS_BRAVE_UI_BRAVE_REWARDS);

    set_bubble_using_cached_web_contents(false);
    set_cached_contents_wrapper(std::move(contents_wrapper));
    cached_contents_wrapper()->ReloadWebContents();

    auto bubble_view = std::make_unique<TipPanelDialogView>(
        anchor_view_, cached_contents_wrapper(), anchor);

    auto weak_ptr = bubble_view->GetWeakPtr();
    views::BubbleDialogDelegateView::CreateBubble(std::move(bubble_view));
    return weak_ptr;
  }

 private:
  const raw_ptr<views::View> anchor_view_;
  const raw_ptr<Profile> profile_;
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
  coordinator_observation_.Observe(coordinator);
}

TipPanelBubbleHost::~TipPanelBubbleHost() = default;

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

  bubble_manager_->ShowBubble();
}

BROWSER_USER_DATA_KEY_IMPL(TipPanelBubbleHost);

}  // namespace brave_rewards

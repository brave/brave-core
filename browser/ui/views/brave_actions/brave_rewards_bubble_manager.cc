/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_rewards_bubble_manager.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_dialog_view.h"
#include "components/grit/brave_components_strings.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace {

// Rewards panel bubble view with support for custom context menu
class RewardsPanelDialogView : public WebUIBubbleDialogView {
 public:
  RewardsPanelDialogView(views::View* anchor_view,
                         BubbleContentsWrapper* contents_wrapper,
                         const absl::optional<gfx::Rect>& anchor_rect)
      : WebUIBubbleDialogView(anchor_view, contents_wrapper, anchor_rect),
        contents_wrapper_(contents_wrapper) {}

  // BubbleContentsWrapper::Host:
  void ShowCustomContextMenu(
      gfx::Point point,
      std::unique_ptr<ui::MenuModel> menu_model) override {
    ConvertPointToScreen(this, &point);
    context_menu_model_ = std::move(menu_model);
    context_menu_runner_ = std::make_unique<views::MenuRunner>(
        context_menu_model_.get(),
        views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU);
    context_menu_runner_->RunMenuAt(
        GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
        views::MenuAnchorPosition::kTopLeft, ui::MENU_SOURCE_MOUSE,
        contents_wrapper_->web_contents()->GetContentNativeView());
  }

  void HideCustomContextMenu() override {
    if (context_menu_runner_) {
      context_menu_runner_->Cancel();
    }
  }

 private:
  raw_ptr<BubbleContentsWrapper> contents_wrapper_;
  std::unique_ptr<views::MenuRunner> context_menu_runner_;
  std::unique_ptr<ui::MenuModel> context_menu_model_;
};

}  // namespace

BraveRewardsBubbleManager::BraveRewardsBubbleManager(views::View* anchor_view,
                                                     Profile* profile)
    : anchor_view_(anchor_view), profile_(profile) {}

BraveRewardsBubbleManager::~BraveRewardsBubbleManager() = default;

base::WeakPtr<WebUIBubbleDialogView>
BraveRewardsBubbleManager::CreateWebUIBubbleDialog(
    const absl::optional<gfx::Rect>& anchor) {
  auto contents_wrapper =
      std::make_unique<BubbleContentsWrapperT<RewardsPanelUI>>(
          GURL(kBraveRewardsPanelURL), profile_, IDS_BRAVE_UI_BRAVE_REWARDS);

  set_bubble_using_cached_web_contents(false);
  set_cached_contents_wrapper(std::move(contents_wrapper));
  cached_contents_wrapper()->ReloadWebContents();

  auto bubble_view = std::make_unique<RewardsPanelDialogView>(
      anchor_view_, cached_contents_wrapper(), anchor);
  bubble_view_ = bubble_view->GetWeakPtr();

  auto weak_ptr = bubble_view->GetWeakPtr();
  views::BubbleDialogDelegateView::CreateBubble(std::move(bubble_view));
  return weak_ptr;
}

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/wayback_machine_page_action_controller.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/page_action/wayback_machine_bubble_view.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/features.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/actions/actions.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_anchor.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"

namespace page_actions {

namespace {

constexpr int kIconSize = 16;
constexpr int kBadgeSize = 8;

// Mirrors the legacy WaybackIconImageSource: draws the base icon plus a small
// badge in the bottom-right corner.
class WaybackIconImageSource : public gfx::CanvasImageSource {
 public:
  WaybackIconImageSource(const gfx::IconDescription& icon,
                         const gfx::IconDescription& badge)
      : CanvasImageSource(gfx::Size(icon.dip_size, icon.dip_size)),
        icon_description_(icon),
        badge_description_(badge) {}
  ~WaybackIconImageSource() override = default;

  // gfx::CanvasImageSource overrides:
  void Draw(gfx::Canvas* canvas) override {
    const gfx::ImageSkia icon_image = gfx::CreateVectorIcon(icon_description_);
    const gfx::ImageSkia badge_image =
        gfx::CreateVectorIcon(badge_description_);
    canvas->DrawImageInt(icon_image, 0, 0);

    // Assume that icon & badge are both square and locate badge on the bottom
    // right of the icon.
    const int badge_position =
        icon_description_.dip_size - badge_description_.dip_size;
    canvas->DrawImageInt(badge_image, badge_position, badge_position);
  }

 private:
  const gfx::IconDescription icon_description_;
  const gfx::IconDescription badge_description_;
};

views::BubbleAnchor GetAnchorForBubble(tabs::TabInterface& tab) {
  auto* bwi = tab.GetBrowserWindowInterface();
  if (!bwi) {
    return {};
  }
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(bwi);
  if (!browser_view) {
    return {};
  }
  auto* toolbar_button_provider = browser_view->toolbar_button_provider();
  if (!toolbar_button_provider) {
    return {};
  }
  return toolbar_button_provider->GetPageActionBubbleAnchor(
      kActionShowWaybackMachine);
}

}  // namespace

WaybackMachinePageActionController::WaybackMachinePageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {}

WaybackMachinePageActionController::~WaybackMachinePageActionController() {
  if (bubble_tracker_.view()) {
    bubble_tracker_.view()->GetWidget()->CloseWithReason(
        views::Widget::ClosedReason::kUnspecified);
  }
}

void WaybackMachinePageActionController::Init() {
  did_activate_subscription_ = tab_->RegisterDidActivate(base::BindRepeating(
      [](WaybackMachinePageActionController* self, tabs::TabInterface* tab) {
        self->AttachToTabHelper(tab->GetContents());
        self->UpdatePageAction(tab->GetContents());
      },
      base::Unretained(this)));
  will_discard_contents_subscription_ =
      tab_->RegisterWillDiscardContents(base::BindRepeating(
          [](WaybackMachinePageActionController* self, tabs::TabInterface*,
             content::WebContents*, content::WebContents* new_contents) {
            self->AttachToTabHelper(new_contents);
            self->UpdatePageAction(new_contents);
          },
          base::Unretained(this)));
  AttachToTabHelper(tab_->GetContents());
  UpdatePageAction(tab_->GetContents());
}

void WaybackMachinePageActionController::ExecuteAction(
    actions::ActionItem* item) {
  ShowBubble(item, /*user_gesture=*/true);
}

WaybackMachineBubbleView*
WaybackMachinePageActionController::GetBubbleViewForTesting() {
  return views::AsViewClass<WaybackMachineBubbleView>(bubble_tracker_.view());
}

void WaybackMachinePageActionController::OnWaybackStateChanged(
    WaybackState state) {
  UpdatePageAction(tab_->GetContents());
  if (state == WaybackState::kNeedToCheck) {
    MaybeAutoShowBubble();
  }
}

void WaybackMachinePageActionController::ShowBubble(actions::ActionItem* item,
                                                    bool user_gesture) {
  content::WebContents* contents = tab_->GetContents();
  if (!contents) {
    return;
  }

  if (bubble_tracker_.view()) {
    return;
  }

  auto* tab_helper = BraveWaybackMachineTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    return;
  }
  const WaybackState state = tab_helper->wayback_state();
  if (state == WaybackState::kInitial || state == WaybackState::kLoaded) {
    return;
  }

  const views::BubbleAnchor anchor = GetAnchorForBubble(tab_.get());
  const views::View* anchor_view = anchor.GetIfView();
  if (!anchor_view || !anchor_view->GetWidget()) {
    return;
  }

  auto bubble =
      std::make_unique<WaybackMachineBubbleView>(anchor, contents, item);
  WaybackMachineBubbleView* bubble_view = bubble.get();
  bubble_tracker_.SetView(bubble_view);

  views::BubbleDialogDelegateView::CreateBubble(std::move(bubble));
  bubble_view->ShowForReason(user_gesture
                                 ? LocationBarBubbleDelegateView::USER_GESTURE
                                 : LocationBarBubbleDelegateView::AUTOMATIC);
}

void WaybackMachinePageActionController::MaybeAutoShowBubble() {
  if (!base::FeatureList::IsEnabled(
          brave_wayback_machine::features::kWaybackMachineAutoShowBubble)) {
    return;
  }
  if (!tab_->IsActivated()) {
    return;
  }

  auto* bwi = tab_->GetBrowserWindowInterface();
  if (!bwi) {
    return;
  }

  auto* root_item = bwi->GetFeatures().GetRootActionItem();
  if (!root_item) {
    return;
  }

  auto* item = actions::ActionManager::Get().FindAction(
      kActionShowWaybackMachine, root_item);
  if (!item) {
    return;
  }

  ShowBubble(item, /*user_gesture=*/false);
}

void WaybackMachinePageActionController::AttachToTabHelper(
    content::WebContents* contents) {
  wayback_state_changed_subscription_ = {};
  if (!contents) {
    return;
  }
  auto* tab_helper = BraveWaybackMachineTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    return;
  }
  wayback_state_changed_subscription_ =
      tab_helper->RegisterWaybackStateChangedCallback(base::BindRepeating(
          &WaybackMachinePageActionController::OnWaybackStateChanged,
          base::Unretained(this)));
}

void WaybackMachinePageActionController::UpdatePageAction(
    content::WebContents* contents) {
  if (!contents) {
    page_action_controller_->Hide(kActionShowWaybackMachine);
    return;
  }

  auto* prefs = user_prefs::UserPrefs::Get(contents->GetBrowserContext());
  if (!IsWaybackMachineEnabled(prefs)) {
    page_action_controller_->Hide(kActionShowWaybackMachine);
    return;
  }

  auto* tab_helper = BraveWaybackMachineTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    page_action_controller_->Hide(kActionShowWaybackMachine);
    return;
  }

  const WaybackState state = tab_helper->wayback_state();
  if (state == WaybackState::kInitial || state == WaybackState::kLoaded) {
    page_action_controller_->Hide(kActionShowWaybackMachine);
    return;
  }

  page_action_controller_->Show(kActionShowWaybackMachine);

  const ui::ColorProvider& color_provider = contents->GetColorProvider();
  if (state != WaybackState::kNotAvailable) {
    page_action_controller_->OverrideImage(
        kActionShowWaybackMachine,
        ui::ImageModel::FromVectorIcon(
            kLeoCalendarTimeIcon,
            color_provider.GetColor(kColorToolbarButtonIcon), kIconSize));
    return;
  }

  const gfx::IconDescription icon_description(
      kLeoCalendarTimeIcon, kIconSize,
      color_provider.GetColor(kColorToolbarButtonIcon));
  const gfx::IconDescription badge_description(
      kLeoDesktopVpnErrorColorIcon, kBadgeSize,
      color_provider.GetColor(kColorWaybackMachineURLNotAvailable));

  gfx::ImageSkia icon_image(
      std::make_unique<WaybackIconImageSource>(icon_description,
                                               badge_description),
      gfx::Size(icon_description.dip_size, icon_description.dip_size));
  page_action_controller_->OverrideImage(
      kActionShowWaybackMachine, ui::ImageModel::FromImageSkia(icon_image));
}

}  // namespace page_actions

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/wayback_machine_page_action_controller.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/page_action/wayback_machine_bubble_view.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_anchor.h"
#include "ui/views/view.h"

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

}  // namespace

WaybackMachinePageActionController::WaybackMachinePageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {}

WaybackMachinePageActionController::~WaybackMachinePageActionController() {
  DetachFromTabHelper(tab_->GetContents());
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
             content::WebContents* old_contents,
             content::WebContents* new_contents) {
            // TabInterface::GetContents() still returns |old_contents| at
            // this point, so both sides of the swap have to be driven by the
            // arguments. Detaching matters: the helper holds a single callback
            // and CHECKs that it was cleared before it's destroyed.
            self->DetachFromTabHelper(old_contents);
            self->AttachToTabHelper(new_contents);
            self->UpdatePageAction(new_contents);
          },
          base::Unretained(this)));
  AttachToTabHelper(tab_->GetContents());
  UpdatePageAction(tab_->GetContents());
}

void WaybackMachinePageActionController::ExecuteAction(
    ToolbarButtonProvider* toolbar_button_provider,
    actions::ActionItem* item) {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    return;
  }
  auto* tab_helper = BraveWaybackMachineTabHelper::FromWebContents(contents);
  if (!tab_helper || tab_helper->active_window().has_value()) {
    return;
  }

  views::View* const anchor_view =
      toolbar_button_provider
          ->GetPageActionBubbleAnchor(kActionShowWaybackMachine)
          .GetIfView();
  if (!anchor_view || !anchor_view->GetWidget()) {
    return;
  }

  WaybackMachineBubbleView::Show(contents, anchor_view, item);
}

void WaybackMachinePageActionController::OnWaybackStateChanged(
    WaybackState state) {
  UpdatePageAction(tab_->GetContents());
}

void WaybackMachinePageActionController::AttachToTabHelper(
    content::WebContents* contents) {
  if (!contents) {
    return;
  }
  auto* tab_helper = BraveWaybackMachineTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    return;
  }
  tab_helper->SetWaybackStateChangedCallback(base::BindRepeating(
      &WaybackMachinePageActionController::OnWaybackStateChanged,
      weak_factory_.GetWeakPtr()));
}

void WaybackMachinePageActionController::DetachFromTabHelper(
    content::WebContents* contents) {
  if (!contents) {
    return;
  }
  if (auto* tab_helper =
          BraveWaybackMachineTabHelper::FromWebContents(contents)) {
    tab_helper->SetWaybackStateChangedCallback(base::NullCallback());
  }
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
  if (state == WaybackState::kInitial) {
    page_action_controller_->Hide(kActionShowWaybackMachine);
    return;
  }

  page_action_controller_->Show(kActionShowWaybackMachine);

  const ui::ColorProvider& color_provider = contents->GetColorProvider();
  if (state != WaybackState::kLoaded && state != WaybackState::kNotAvailable) {
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

  const bool loaded = state == WaybackState::kLoaded;
  const ui::ColorId badge_color_id = loaded
                                         ? kColorWaybackMachineURLLoaded
                                         : kColorWaybackMachineURLNotAvailable;
  const gfx::VectorIcon& badge_icon =
      loaded ? kLeoDesktopVpnOnColorIcon : kLeoDesktopVpnErrorColorIcon;
  const gfx::IconDescription badge_description(
      badge_icon, kBadgeSize, color_provider.GetColor(badge_color_id));

  gfx::ImageSkia icon_image(
      std::make_unique<WaybackIconImageSource>(icon_description,
                                               badge_description),
      gfx::Size(icon_description.dip_size, icon_description.dip_size));
  page_action_controller_->OverrideImage(
      kActionShowWaybackMachine, ui::ImageModel::FromImageSkia(icon_image));
}

}  // namespace page_actions

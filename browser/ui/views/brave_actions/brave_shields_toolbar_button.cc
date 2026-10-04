// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_toolbar_button.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

namespace {

class ShieldsToolbarHighlightPathGenerator
    : public views::HighlightPathGenerator {
  SkPath GetHighlightPath(const views::View* view) override {
    return SkPath::Rect(gfx::RectToSkRect(view->GetLocalBounds()));
  }
};

}  // namespace

BraveShieldsToolbarButton::BraveShieldsToolbarButton(
    BrowserWindowInterface* browser_window_interface,
    CreateWebUIBubbleManagerCallback create_bubble_manager_callback)
    : ToolbarButton(views::Button::PressedCallback()),
      controller_(std::make_unique<BraveShieldsActionController>(
          browser_window_interface,
          std::move(create_bubble_manager_callback))) {
  SetText(std::u16string());
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  // Use the same element identifier as BraveShieldsActionView so that we can
  // find either of them in the BrowserElementsViews.
  SetProperty(views::kElementIdentifierKey,
              BraveShieldsActionView::kShieldsActionIcon);
  SetBorder(nullptr);

  // Match BraveShieldsActionView: MenuButtonController handles press so the
  // bubble closes when the button is clicked while already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveShieldsToolbarButton::ButtonPressed,
                          weak_ptr_factory_.GetWeakPtr()),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  SetButtonController(std::move(menu_button_controller));

  controller_->SetOnStateChanged(
      base::BindRepeating(&BraveShieldsToolbarButton::OnControllerStateChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  controller_->SetIconStyle(
      BraveShieldsActionController::IconStyle::kWebAppTitleBar);
  controller_->SetAnchorView(this);

  views::HighlightPathGenerator::Install(
      this, std::make_unique<ShieldsToolbarHighlightPathGenerator>());

  Update();
}

BraveShieldsToolbarButton::~BraveShieldsToolbarButton() = default;

void BraveShieldsToolbarButton::ButtonPressed() {
  controller_->OnButtonPressed();
}

void BraveShieldsToolbarButton::OnControllerStateChanged() {
  Update();
}

void BraveShieldsToolbarButton::Update() {
  controller_->RefreshButtonImages(this);
  PreferredSizeChanged();
}

views::Widget* BraveShieldsToolbarButton::GetBubbleWidget() {
  return controller_->GetBubbleWidget();
}

std::u16string BraveShieldsToolbarButton::GetRenderedTooltipText(
    const gfx::Point& p) const {
  return controller_->GetTooltipText();
}

void BraveShieldsToolbarButton::OnThemeChanged() {
  ToolbarButton::OnThemeChanged();
  // Bitmap icon already encodes state; re-resolve image from controller for
  // any color provider updates.
  Update();
}

BEGIN_METADATA(BraveShieldsToolbarButton)
END_METADATA

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <utility>

#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/views/controls/highlight_path_generator.h"

#define ToolbarButton ToolbarButton_ChromiumImpl
#include "src/chrome/browser/ui/views/toolbar/toolbar_button.cc"
#undef ToolbarButton

void ToolbarButton_ChromiumImpl::SetMenuModel(
    std::unique_ptr<ui::MenuModel> model) {
  model_ = std::move(model);
}

bool ToolbarButton_ChromiumImpl::HasVectorIcons() const {
  return vector_icons_.has_value();
}

const gfx::VectorIcon& ToolbarButton_ChromiumImpl::GetVectorIcon() const {
  CHECK(HasVectorIcons());
  return vector_icons_->icon;
}

const gfx::VectorIcon& ToolbarButton_ChromiumImpl::GetVectorTouchIcon() const {
  CHECK(HasVectorIcons());
  return vector_icons_->touch_icon;
}

ToolbarButton::~ToolbarButton() {
  views::InkDrop::Get(this)->GetInkDrop()->RemoveObserver(this);
}

void ToolbarButton::OnThemeChanged() {
  ToolbarButton_ChromiumImpl::OnThemeChanged();

  // Reset ink drop config as inkdrop has different config per themes.
  // Don't need to remove the observation for previous inkdrop before
  // destroying it as it's unchecked observer list.
  ConfigureInkDropForToolbar(this);
  views::InkDrop::Get(this)->GetInkDrop()->AddObserver(this);

  SetHighlighted(activated_);
}

void ToolbarButton::InkDropRippleAnimationEnded(views::InkDropState state) {
  OnInkDropStateChanged(state);
}

void ToolbarButton::OnInkDropStateChanged(views::InkDropState state) {
  // Use different color for icon when activated.
  activated_ = state == views::InkDropState::ACTIVATED;

  if (!activated_) {
    // Set upstream colors for deactivated state. When called from the button
    // destructor, the color provider may no longer be there.
    if (GetColorProvider()) {
      UpdateIcon();
    }
    return;
  }

  if (!HasVectorIcons()) {
    return;
  }

  // Use different icon color when button is activated.
  if (const auto* color_provider = GetColorProvider()) {
    const auto activated_color =
        color_provider->GetColor(kColorToolbarButtonActivated);
    UpdateIconsWithColors(
        ui::TouchUiController::Get()->touch_ui() ? GetVectorTouchIcon()
                                                 : GetVectorIcon(),
        activated_color, activated_color, activated_color, activated_color);
  }
}

BEGIN_METADATA(ToolbarButton)
END_METADATA

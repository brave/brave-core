/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"

#include <memory>

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/brave_tab_search_bubble_host.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveTabSearchButton::BraveTabSearchButton(
    TabStripController* tab_strip_controller,
    BrowserWindowInterface* browser_window_interface,
    Edge flat_edge)
    : TabSearchButton(tab_strip_controller,
                      browser_window_interface,
                      flat_edge) {
  // Resetting the tab search bubble host first, to avoid a dangling in
  // `BraveTabSearchButton`, triggered `TabSearchBubbleHost` calling
  // `SetButtonController` and in the process destroying the still alive
  // `MenuButtonController` through a move assignment, leaving a dangliing
  // pointer behind.
  tab_search_bubble_host_ = nullptr;

  tab_search_bubble_host_ = std::make_unique<BraveTabSearchBubbleHost>(
      this, browser_window_interface);

  // Apply toolbar's icon color to search button.
  SetForegroundFrameActiveColorId(kColorToolbarButtonIcon);
  SetForegroundFrameInactiveColorId(kColorToolbarButtonIcon);
}

BraveTabSearchButton::~BraveTabSearchButton() = default;

void BraveTabSearchButton::SetBubbleArrow(views::BubbleBorder::Arrow arrow) {
  static_cast<BraveTabSearchBubbleHost*>(tab_search_bubble_host_.get())
      ->SetBubbleArrow(arrow);
}

void BraveTabSearchButton::UpdateColors() {
  TabSearchButton::UpdateColors();

  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return;
  }

  // Use a custom icon for tab search.
  constexpr int kIconSize = 18;
  const ui::ImageModel icon_image_model = ui::ImageModel::FromVectorIcon(
      vector_icons::kCaretDownIcon, GetForegroundColor(), kIconSize);
  SetImageModel(views::Button::STATE_NORMAL, icon_image_model);
  SetImageModel(views::Button::STATE_HOVERED, icon_image_model);
  SetImageModel(views::Button::STATE_PRESSED, icon_image_model);

  // Unset any backgrounds or borders.
  SetBorder(nullptr);
  SetBackground(nullptr);
}

int BraveTabSearchButton::GetCornerRadius() const {
  return TabStripControlButton::GetCornerRadius();
}

BEGIN_METADATA(BraveTabSearchButton)
END_METADATA

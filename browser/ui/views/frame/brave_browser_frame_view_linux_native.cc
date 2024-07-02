/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_linux_native.h"

#include <numeric>
#include <string>

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/pref_names.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/window/caption_button_layout_constants.h"
#include "ui/views/window/window_button_order_provider.h"

namespace {

ui::NavButtonProvider::ButtonState ButtonStateToNavButtonProviderState(
    views::Button::ButtonState state) {
  switch (state) {
    case views::Button::STATE_NORMAL:
      return ui::NavButtonProvider::ButtonState::kNormal;
    case views::Button::STATE_HOVERED:
      return ui::NavButtonProvider::ButtonState::kHovered;
    case views::Button::STATE_PRESSED:
      return ui::NavButtonProvider::ButtonState::kPressed;
    case views::Button::STATE_DISABLED:
      return ui::NavButtonProvider::ButtonState::kDisabled;

    case views::Button::STATE_COUNT:
    default:
      NOTREACHED_IN_MIGRATION();
      return ui::NavButtonProvider::ButtonState::kNormal;
  }
}

}  // namespace

BraveBrowserFrameViewLinuxNative::BraveBrowserFrameViewLinuxNative(
    BrowserFrame* frame,
    BrowserView* browser_view,
    BrowserFrameViewLayoutLinuxNative* layout,
    std::unique_ptr<ui::NavButtonProvider> nav_button_provider)
    : BrowserFrameViewLinuxNative(frame,
                                  browser_view,
                                  layout,
                                  std::move(nav_button_provider)) {}

BraveBrowserFrameViewLinuxNative::~BraveBrowserFrameViewLinuxNative() = default;

void BraveBrowserFrameViewLinuxNative::MaybeUpdateCachedFrameButtonImages() {
  auto* browser = browser_view()->browser();
  DCHECK(browser);

  if (!tabs::utils::ShouldShowVerticalTabs(browser) ||
      tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser)) {
    BrowserFrameViewLinuxNative::MaybeUpdateCachedFrameButtonImages();
    UpdateLeadingTrailingCaptionButtonWidth();
    return;
  }

  // In order to lay out window caption buttons over toolbar, we should set
  // height as tall as button's on toolbar
  DrawFrameButtonParams params{
      .top_area_height = GetLayoutConstant(TOOLBAR_BUTTON_HEIGHT) +
                         GetLayoutInsets(TOOLBAR_BUTTON).height() +
                         GetTopAreaHeight() -
                         layout()->FrameEdgeInsets(!IsMaximized()).top(),
      .maximized = IsMaximized(),
      .active = ShouldPaintAsActive()};

  if (cache_ == params) {
    return;
  }

  cache_ = params;
  nav_button_provider_->RedrawImages(cache_.top_area_height, cache_.maximized,
                                     cache_.active);

  for (auto type : {
           ui::NavButtonProvider::FrameButtonDisplayType::kMinimize,
           IsMaximized()
               ? ui::NavButtonProvider::FrameButtonDisplayType::kRestore
               : ui::NavButtonProvider::FrameButtonDisplayType::kMaximize,
           ui::NavButtonProvider::FrameButtonDisplayType::kClose,
       }) {
    for (size_t state = 0; state < views::Button::STATE_COUNT; state++) {
      views::Button::ButtonState button_state =
          static_cast<views::Button::ButtonState>(state);
      views::Button* button = GetButtonFromDisplayType(type);
      DCHECK_EQ(std::string(views::ImageButton::kViewClassName),
                button->GetClassName());
      static_cast<views::ImageButton*>(button)->SetImageModel(
          button_state,
          ui::ImageModel::FromImageSkia(nav_button_provider_->GetImage(
              type, ButtonStateToNavButtonProviderState(button_state))));
    }
  }

  UpdateLeadingTrailingCaptionButtonWidth();
}

void BraveBrowserFrameViewLinuxNative::Layout(PassKey) {
  LayoutSuperclass<BrowserFrameViewLinuxNative>(this);

  UpdateLeadingTrailingCaptionButtonWidth();
}

views::Button* BraveBrowserFrameViewLinuxNative::FrameButtonToButton(
    views::FrameButton frame_button) {
  switch (frame_button) {
    case views::FrameButton::kMinimize:
      return minimize_button();
    case views::FrameButton::kMaximize:
      return IsMaximized() ? restore_button() : maximize_button();
    case views::FrameButton::kClose:
      return close_button();
  }
  NOTREACHED_IN_MIGRATION();
  return nullptr;
}

void BraveBrowserFrameViewLinuxNative::
    UpdateLeadingTrailingCaptionButtonWidth() {
  auto* browser = browser_view()->browser();
  DCHECK(browser);
  std::pair<int, int> new_leading_trailing_caption_button_width;
  if (tabs::utils::ShouldShowVerticalTabs(browser) &&
      !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser)) {
    auto* window_order_provider =
        views::WindowButtonOrderProvider::GetInstance();
    const auto leading_buttons = window_order_provider->leading_buttons();
    const auto trailing_buttons = window_order_provider->trailing_buttons();

    const auto leading_button_width = std::accumulate(
        leading_buttons.begin(), leading_buttons.end(), 0,
        [this](int max_right, auto frame_button) {
          auto* button = FrameButtonToButton(frame_button);
          if (!button || !button->GetVisible() || button->bounds().IsEmpty()) {
            return max_right;
          }

          if (auto current_right = button->bounds().right();
              current_right > max_right) {
            return current_right;
          }

          return max_right;
        });

    const auto trailing_button_width =
        width() -
        std::accumulate(trailing_buttons.begin(), trailing_buttons.end(),
                        width(), [this](int min_left, auto frame_button) {
                          auto* button = FrameButtonToButton(frame_button);
                          if (!button || !button->GetVisible() ||
                              button->bounds().IsEmpty()) {
                            return min_left;
                          }

                          if (int current_left = button->bounds().x();
                              current_left < min_left) {
                            return current_left;
                          }

                          return min_left;
                        });
    new_leading_trailing_caption_button_width = {leading_button_width,
                                                 trailing_button_width};
  }

  if (leading_trailing_caption_button_width_ ==
      new_leading_trailing_caption_button_width) {
    return;
  }

  leading_trailing_caption_button_width_ =
      new_leading_trailing_caption_button_width;

  // Notify toolbar view that caption button's width changed so that it can
  // make space for caption buttons.
  static_cast<BraveToolbarView*>(browser_view()->toolbar())
      ->UpdateHorizontalPadding();
}

// Unfortunately, BrowserFrameViewLinux(Native) doesn't declare metadata.
// OpaqueBrowserFrameView is the nearest ancestor.
BEGIN_METADATA(BraveBrowserFrameViewLinuxNative)
END_METADATA

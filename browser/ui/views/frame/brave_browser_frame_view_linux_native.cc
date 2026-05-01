/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_linux_native.h"

#include <string>

#include "base/check.h"
#include "base/notreached.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "build/build_config.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_layout_linux_native.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/image_button.h"

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
      return ui::NavButtonProvider::ButtonState::kNormal;
  }
  NOTREACHED();
}

}  // namespace

BraveBrowserFrameViewLinuxNative::BraveBrowserFrameViewLinuxNative(
    BrowserWidget* frame,
    BrowserView* browser_view,
    BrowserFrameViewLayoutLinuxNative* layout,
    std::unique_ptr<ui::NavButtonProvider> nav_button_provider)
    : BrowserFrameViewLinuxNative(frame,
                                  browser_view,
                                  layout,
                                  std::move(nav_button_provider)) {}

BraveBrowserFrameViewLinuxNative::~BraveBrowserFrameViewLinuxNative() = default;

void BraveBrowserFrameViewLinuxNative::PaintRestoredFrameBorder(
    gfx::Canvas* canvas) const {
  auto* browser = GetBrowserView()->browser();
  CHECK(browser);

  if (!tabs::utils::ShouldShowBraveVerticalTabs(browser) ||
      tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser)) {
    BrowserFrameViewLinuxNative::PaintRestoredFrameBorder(canvas);
    return;
  }

  // For Brave vertical tabs w/o title, GetTopAreaHeight() returns
  // NonClientTopHeight(false) which equals only the shadow thickness F.
  // PaintWindowFrame then computes top_area_height_px = ClampCeil(F * scale)
  // - shadow_px = 0, producing a zero-height headerbar bitmap that crashes in
  // CairoSurface::CairoSurface().
  // Add the toolbar height here to avoid crashing from gtk's frame painting.
  // As we don't need any visible rect above toolbar, maybe we don't need to
  // call PaintHeaderbar but it crashed with zero-height.
  int top_area_height = GetTopAreaHeight();
  if (auto* const toolbar = GetBrowserView()->toolbar()) {
    top_area_height += toolbar->GetPreferredSize().height();
  }
  layout_->GetFrameProvider()->PaintWindowFrame(
      canvas, GetLocalBounds(), top_area_height, ShouldPaintAsActive(),
      GetInputInsets());
}

void BraveBrowserFrameViewLinuxNative::MaybeUpdateCachedFrameButtonImages() {
  auto* browser = GetBrowserView()->browser();
  DCHECK(browser);

  if (!tabs::utils::ShouldShowBraveVerticalTabs(browser) ||
      tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser)) {
    BrowserFrameViewLinuxNative::MaybeUpdateCachedFrameButtonImages();
    return;
  }

  // In order to lay out window caption buttons over toolbar, we should set
  // height as tall as button's on toolbar
  DrawFrameButtonParams params{
      .top_area_height =
          GetLayoutConstant(LayoutConstant::kToolbarButtonHeight) +
          GetLayoutInsets(TOOLBAR_BUTTON).height() + GetTopAreaHeight() -
          layout()->FrameEdgeInsets(!IsMaximized()).top(),
      .maximized = IsMaximized(),
      .active = ShouldPaintAsActive()};

  if (cache_ == params) {
    return;
  }

  cache_ = params;
  nav_button_provider_->RedrawImages(cache_->top_area_height, cache_->maximized,
                                     cache_->active);

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
}

void BraveBrowserFrameViewLinuxNative::Layout(PassKey) {
  LayoutSuperclass<BrowserFrameViewLinuxNative>(this);

  static_cast<BraveToolbarView*>(GetBrowserView()->toolbar())
      ->UpdateHorizontalPadding();
}

// Unfortunately, BrowserFrameViewLinux(Native) doesn't declare metadata.
// OpaqueBrowserFrameView is the nearest ancestor.
BEGIN_METADATA(BraveBrowserFrameViewLinuxNative)
END_METADATA

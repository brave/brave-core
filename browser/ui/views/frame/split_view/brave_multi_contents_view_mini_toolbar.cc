/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"

#include <memory>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_outline.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_container_outline.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"

namespace {
// Upstream keeps its own copy of this file-local, so we cannot share it. The
// notch radius we do share: it belongs to the outline, which draws it.
constexpr int kMiniToolbarContentPadding = 4;
}  // namespace

// Declared by chromium_src/chrome/browser/ui/views/frame/
// contents_container_view.cc so that upstream's constructor builds our toolbar
// from the start, instead of us replacing the one it would have created.
std::unique_ptr<MultiContentsViewMiniToolbar> CreateBraveMiniToolbar(
    BrowserView* browser_view,
    ContentsWebView* contents_view) {
  return std::make_unique<BraveMultiContentsViewMiniToolbar>(browser_view,
                                                             contents_view);
}

// static
const gfx::VectorIcon& MultiContentsViewMiniToolbar::GetMoreVerticalIcon() {
  return kLeoMoreVerticalIcon;
}

// static
BraveMultiContentsViewMiniToolbar* BraveMultiContentsViewMiniToolbar::From(
    MultiContentsViewMiniToolbar* toolbar) {
  return static_cast<BraveMultiContentsViewMiniToolbar*>(toolbar);
}

BraveMultiContentsViewMiniToolbar::~BraveMultiContentsViewMiniToolbar() =
    default;

void BraveMultiContentsViewMiniToolbar::SetAlwaysShowDomain(
    bool always_show_domain) {
  always_show_domain_ = always_show_domain;
}

void BraveMultiContentsViewMiniToolbar::SetStyle(Style style) {
  if (style_ == style) {
    return;
  }

  style_ = style;

  // A web panel is not a tab the user manages from the strip, so closing it has
  // to go through the panel controller: that animates the panel out and leaves
  // no "recently closed" entry for its pinned tab.
  close_button_->SetCallback(
      style_ == Style::kWebPanel
          ? base::BindRepeating(
                &BraveMultiContentsViewMiniToolbar::CloseWebPanel,
                base::Unretained(this))
          : base::BindRepeating(&MultiContentsViewMiniToolbar::CloseCurrentView,
                                base::Unretained(this)));
}

void BraveMultiContentsViewMiniToolbar::UpdateContents() {
  MultiContentsViewMiniToolbar::UpdateContents();

  // Display as brave scheme.
  std::u16string domain{domain_label_->GetText()};
  if (brave_utils::ReplaceChromeToBraveScheme(domain)) {
    domain_label_->SetText(domain);
  }
}

void BraveMultiContentsViewMiniToolbar::UpdateState(bool is_active,
                                                    bool is_highlighted) {
  const bool domain_visible = !is_active || always_show_domain_;
  MultiContentsViewMiniToolbar::UpdateState(!domain_visible, is_highlighted);

  if (!GetVisible()) {
    return;
  }

  is_active_ = is_active;

  // A standalone contents area is not closable from here; it is the whole tab.
  close_button_->SetVisible(style_ != Style::kStandalone);

  // The leading margin must clear the curved inner side when the favicon is
  // displayed.
  const int leading_margin = domain_visible
                                 ? ContentsContainerOutline::kCornerRadius * 2
                                 : ContentsContainerOutline::kCornerRadius +
                                       kMiniToolbarContentPadding;

  // Without a split border, the trailing child needs its own padding.
  int trailing_margin = ContentsContainerOutline::kCornerRadius;
  if (UsesSplitBorder()) {
    // Otherwise clear the ring that BraveContentsContainerOutline strokes down
    // this side.
    using OutlineStyle = BraveContentsContainerOutline::Style;
    const int stroke = BraveContentsContainerOutline::GetStrokeThickness(
        is_active_ ? OutlineStyle::kSplitActive : OutlineStyle::kSplitInactive);
    trailing_margin = domain_visible ? stroke : stroke * 2;
  }

  static_cast<views::FlexLayout*>(GetLayoutManager())
      ->SetInteriorMargin(gfx::Insets::TLBR(
          ContentsContainerOutline::kCornerRadius + kMiniToolbarContentPadding,
          leading_margin, kMiniToolbarContentPadding, trailing_margin));
}

bool BraveMultiContentsViewMiniToolbar::UsesSplitBorder() const {
  return style_ != Style::kStandalone;
}

void BraveMultiContentsViewMiniToolbar::CloseWebPanel() {
  auto* sidebar_controller =
      sidebar::SidebarController::From(browser_view_->browser());
  CHECK(sidebar_controller);
  auto* web_panel_controller = sidebar_controller->GetWebPanelController();
  CHECK(web_panel_controller);
  web_panel_controller->CloseWebPanel();
}

BEGIN_METADATA(BraveMultiContentsViewMiniToolbar)
END_METADATA

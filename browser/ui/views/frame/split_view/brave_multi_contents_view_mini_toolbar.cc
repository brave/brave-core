/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"

#include <cmath>
#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/browser/ui/tabs/brave_split_tab_menu_model.h"
#include "brave/browser/ui/views/frame/edge_reveal/edge_hover_detector.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_outline.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_container_outline.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"

namespace {
// Upstream keeps its own copy of this file-local, so we cannot share it. The
// notch radius we do share: it belongs to the outline, which draws it.
constexpr int kMiniToolbarContentPadding = 4;
constexpr base::TimeDelta kSlideAnimationDuration = base::Milliseconds(150);
constexpr base::TimeDelta kHoverExitDelay = base::Milliseconds(200);
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
std::unique_ptr<SplitTabMenuModel>
MultiContentsViewMiniToolbar::CreateBraveSplitTabMenuModel(
    TabStripModel* tab_strip_model,
    SplitTabMenuModel::MenuSource source,
    int split_tab_index) {
  return std::make_unique<BraveSplitTabMenuModel>(tab_strip_model, source,
                                                  split_tab_index);
}

// static
BraveMultiContentsViewMiniToolbar* BraveMultiContentsViewMiniToolbar::From(
    MultiContentsViewMiniToolbar* toolbar) {
  return static_cast<BraveMultiContentsViewMiniToolbar*>(toolbar);
}

BraveMultiContentsViewMiniToolbar::BraveMultiContentsViewMiniToolbar(
    BrowserView* browser_view,
    ContentsWebView* web_view)
    : MultiContentsViewMiniToolbar(browser_view, web_view) {
  slide_animation_.SetSlideDuration(kSlideAnimationDuration);
  slide_animation_.Reset(1.0);
}

BraveMultiContentsViewMiniToolbar::~BraveMultiContentsViewMiniToolbar() {
  hover_detector_.reset();
  slide_animation_.Stop();
}

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

void BraveMultiContentsViewMiniToolbar::SetHideOnHover(bool hide_on_hover) {
  if (hide_on_hover_ == hide_on_hover) {
    return;
  }

  hide_on_hover_ = hide_on_hover;
  UpdateHoverDetector();
}

double BraveMultiContentsViewMiniToolbar::GetRevealFraction() const {
  return slide_animation_.GetCurrentValue();
}

int BraveMultiContentsViewMiniToolbar::GetSlideOffset() const {
  return static_cast<int>(std::round((1.0 - GetRevealFraction()) * height()));
}

base::CallbackListSubscription
BraveMultiContentsViewMiniToolbar::RegisterRevealFractionChanged(
    base::RepeatingClosure callback) {
  return reveal_fraction_changed_callbacks_.Add(std::move(callback));
}

void BraveMultiContentsViewMiniToolbar::OnHoverStateChangedForTesting(
    bool hovering) {
  OnHoverStateChanged(hovering);
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

void BraveMultiContentsViewMiniToolbar::OnBoundsChanged(
    const gfx::Rect& previous_bounds) {
  MultiContentsViewMiniToolbar::OnBoundsChanged(previous_bounds);
  if (GetRevealFraction() < 1.0) {
    UpdateSlideState();
  }
}

void BraveMultiContentsViewMiniToolbar::VisibilityChanged(
    views::View* starting_from,
    bool is_visible) {
  MultiContentsViewMiniToolbar::VisibilityChanged(starting_from, is_visible);
  UpdateHoverDetector();
}

void BraveMultiContentsViewMiniToolbar::AddedToWidget() {
  MultiContentsViewMiniToolbar::AddedToWidget();
  UpdateHoverDetector();
}

void BraveMultiContentsViewMiniToolbar::RemovedFromWidget() {
  ResetHoverHide();
  MultiContentsViewMiniToolbar::RemovedFromWidget();
}

void BraveMultiContentsViewMiniToolbar::AnimationProgressed(
    const gfx::Animation* animation) {
  UpdateSlideState();
}

void BraveMultiContentsViewMiniToolbar::AnimationEnded(
    const gfx::Animation* animation) {
  UpdateSlideState();
}

bool BraveMultiContentsViewMiniToolbar::UsesSplitBorder() const {
  return style_ != Style::kStandalone;
}

void BraveMultiContentsViewMiniToolbar::UpdateHoverDetector() {
  if (hide_on_hover_ && GetVisible() && GetWidget()) {
    if (!hover_detector_) {
      hover_detector_ = std::make_unique<EdgeHoverDetector>(
          GetWidget(),
          base::BindRepeating(&BraveMultiContentsViewMiniToolbar::IsHoverPoint,
                              base::Unretained(this)),
          base::BindRepeating(
              &BraveMultiContentsViewMiniToolbar::OnHoverStateChanged,
              base::Unretained(this)),
          EdgeHoverDetector::kMouseEnterDelay, kHoverExitDelay);
    }
  } else {
    ResetHoverHide();
  }
}

void BraveMultiContentsViewMiniToolbar::ResetHoverHide() {
  hover_detector_.reset();
  const bool needs_update = slide_animation_.GetCurrentValue() != 1.0 ||
                            slide_animation_.is_animating();
  slide_animation_.Reset(1.0);
  SetCanProcessEventsWithinSubtree(true);
  if (needs_update) {
    UpdateSlideState();
  }
}

bool BraveMultiContentsViewMiniToolbar::IsHoverPoint(
    const gfx::Point& screen_point) const {
  if (!parent()) {
    return false;
  }
  gfx::Rect resting_bounds = GetMirroredBounds();
  resting_bounds.Offset(parent()->GetBoundsInScreen().OffsetFromOrigin());
  return resting_bounds.Contains(screen_point);
}

void BraveMultiContentsViewMiniToolbar::OnHoverStateChanged(bool hovering) {
  SetCanProcessEventsWithinSubtree(!hovering);
  if (hovering) {
    slide_animation_.Hide();
  } else {
    slide_animation_.Show();
  }
}

void BraveMultiContentsViewMiniToolbar::UpdateSlideState() {
  if (layer()) {
    const double reveal_fraction = GetRevealFraction();
    const int dy = GetSlideOffset();
    if (reveal_fraction == 0.0 || (height() > 0 && dy >= height())) {
      layer()->SetOpacity(0.0f);
      layer()->SetTransform(gfx::Transform());
      layer()->SetClipRect(gfx::Rect());
    } else if (dy <= 0) {
      layer()->SetOpacity(1.0f);
      layer()->SetTransform(gfx::Transform());
      layer()->SetClipRect(gfx::Rect());
    } else {
      layer()->SetOpacity(1.0f);
      layer()->SetTransform(gfx::Transform::MakeTranslation(0, dy));
      layer()->SetClipRect(gfx::Rect(0, 0, width(), height() - dy));
    }
  }
  reveal_fraction_changed_callbacks_.Notify();
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

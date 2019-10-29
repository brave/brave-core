// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_rewards_action_stub_view.h"

#include <string>
#include <memory>
#include <utility>

#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"  // NOLINT
#include "brave/browser/ui/brave_actions/constants.h"
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_extension_resources.h"  // NOLINT
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia_source.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"

namespace {
  constexpr SkColor kRewardsBadgeBg = SkColorSetRGB(0xfb, 0x54, 0x2b);
  const std::string kRewardsInitialBadgeText = "1";
}

BraveRewardsActionStubView::BraveRewardsActionStubView(
    BraveRewardsActionStubView::Delegate* delegate)
    : LabelButton(this, base::string16()),
      delegate_(delegate) {
  SetInkDropMode(InkDropMode::ON);
  set_has_ink_drop_action_on_click(true);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  set_ink_drop_visible_opacity(kToolbarInkDropVisibleOpacity);
  // Create badge-and-image source like an extension icon would
  auto preferred_size = GetPreferredSize();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  std::unique_ptr<IconWithBadgeImageSource> image_source(
      new BraveActionIconWithBadgeImageSource(preferred_size));
  // Set icon on badge using actual extension icon resource
  const auto image = gfx::Image(
      rb.GetImageNamed(IDR_BRAVE_REWARDS_ICON_64).AsImageSkia()
          .DeepCopy());
  image_source->SetIcon(image);
  // Set text on badge
  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  badge.reset(new IconWithBadgeImageSource::Badge(
          kRewardsInitialBadgeText,
          SK_ColorWHITE,
          kRewardsBadgeBg));
  image_source->SetBadge(std::move(badge));
  image_source->set_paint_page_action_decoration(false);
  gfx::ImageSkia icon(gfx::Image(
      gfx::ImageSkia(
          std::move(image_source),
          preferred_size))
      .AsImageSkia());
  // Use badge-and-icon source for button's image in all states
  SetImage(views::Button::STATE_NORMAL, icon);
  // Set the highlight path for the toolbar button,
  // making it inset so that the badge can show outside it in the
  // fake margin on the right that we are creating.
  gfx::Insets highlight_insets(0, 0, 0, brave_actions::kBraveActionRightMargin);
  gfx::Rect rect(preferred_size);
  rect.Inset(highlight_insets);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_MAXIMUM, rect.size());
  auto path = std::make_unique<SkPath>();
  path->addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  SetProperty(views::kHighlightPathKey, path.release());
}

BraveRewardsActionStubView::~BraveRewardsActionStubView() {}

void BraveRewardsActionStubView::ButtonPressed(
    Button* sender, const ui::Event& event) {
  delegate_->OnRewardsStubButtonClicked();
}

gfx::Size BraveRewardsActionStubView::CalculatePreferredSize() const {
  return delegate_->GetToolbarActionSize();
}

std::unique_ptr<views::LabelButtonBorder> BraveRewardsActionStubView::
    CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(
      gfx::Insets(0, 0, 0, 0));
  return border;
}

SkColor BraveRewardsActionStubView::GetInkDropBaseColor() const {
  return GetToolbarInkDropBaseColor(this);
}

std::unique_ptr<views::InkDropHighlight>
BraveRewardsActionStubView::CreateInkDropHighlight() const {
  return CreateToolbarInkDropHighlight(this);
}

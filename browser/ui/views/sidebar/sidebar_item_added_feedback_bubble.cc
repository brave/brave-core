/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_added_feedback_bubble.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace {
constexpr int kFadeoutDurationInMs = 500;
}  // namespace

// static
views::Widget* SidebarItemAddedFeedbackBubble::Create(
    views::View* anchor_view,
    views::View* items_contents_view) {
  auto* delegate =
      new SidebarItemAddedFeedbackBubble(anchor_view, items_contents_view);
  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(delegate);
  auto* frame_view = delegate->GetBubbleFrameView();
  frame_view->bubble_border()->set_md_shadow_elevation(
      ChromeLayoutProvider::Get()->GetShadowElevationMetric(
          views::Emphasis::kHigh));
  frame_view->SetContentMargins(gfx::Insets::VH(10, 18));
  frame_view->SetDisplayVisibleArrow(true);
  delegate->set_adjust_if_offscreen(true);
  delegate->SizeToContents();
  frame_view->SetCornerRadius(6);

  return bubble;
}

SidebarItemAddedFeedbackBubble::SidebarItemAddedFeedbackBubble(
    views::View* anchor_view,
    views::View* items_contents_view)
    : BubbleDialogDelegateView(anchor_view,
                               views::BubbleBorder::LEFT_CENTER,
                               views::BubbleBorder::STANDARD_SHADOW),
      animation_(base::Milliseconds(kFadeoutDurationInMs), 60, this) {
  // This bubble uses same color for all themes.
  constexpr SkColor kBubbleBackground = SkColorSetRGB(0x33, 0x9A, 0xF0);
  set_color(kBubbleBackground);
  set_margins(gfx::Insets());
  set_title_margins(gfx::Insets());
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));

  AddChildViews();
  observed_.Observe(items_contents_view);
}

SidebarItemAddedFeedbackBubble::~SidebarItemAddedFeedbackBubble() = default;

void SidebarItemAddedFeedbackBubble::OnWidgetVisibilityChanged(
    views::Widget* widget,
    bool visible) {
  BubbleDialogDelegateView::OnWidgetVisibilityChanged(widget, visible);
  if (visible && !fade_timer_.IsRunning()) {
    constexpr int kFadeoutStartTimeInMs = 2500;
    fade_timer_.Start(
        FROM_HERE, base::Milliseconds(kFadeoutStartTimeInMs),
        base::BindOnce(&gfx::Animation::Start, base::Unretained(&animation_)));
  }
}

void SidebarItemAddedFeedbackBubble::OnWidgetDestroying(views::Widget* widget) {
  fade_timer_.Stop();
  animation_.Stop();
}

void SidebarItemAddedFeedbackBubble::AnimationProgressed(
    const gfx::Animation* animation) {
  GetWidget()->GetLayer()->SetOpacity(animation->CurrentValueBetween(1.0, 0.0));
}

void SidebarItemAddedFeedbackBubble::AnimationEnded(
    const gfx::Animation* animation) {
  AnimationProgressed(animation);
  GetWidget()->Close();
}

void SidebarItemAddedFeedbackBubble::OnViewBoundsChanged(View* observed_view) {
  // Re-position as SidebarItemsContentsView's bounds changed. This change also
  // causes' anchor's position from widget point of view.
  SizeToContents();
}

void SidebarItemAddedFeedbackBubble::AddChildViews() {
  auto* layout_manager = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout_manager->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  auto* first_row = AddChildView(std::make_unique<views::View>());
  constexpr int kChildSpacing = 6;
  first_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kChildSpacing));
  auto* image = first_row->AddChildView(std::make_unique<views::ImageView>());
  image->SetImage(
      gfx::CreateVectorIcon(kSidebarItemAddedCheckIcon, SK_ColorWHITE));
  // Use 12pt and 600 weight.
  auto* label = first_row->AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ADDED_FEEDBACK_TITLE_1),
      views::Label::CustomFont(
          {views::Label::GetDefaultFontList().DeriveWithWeight(
              gfx::Font::Weight::SEMIBOLD)})));
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(SK_ColorWHITE);

  // 11pt (1 decreased from default(12)) and 500 weight.
  label = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ADDED_FEEDBACK_TITLE_2),
      views::Label::CustomFont(
          {views::Label::GetDefaultFontList()
               .DeriveWithSizeDelta(-1)
               .DeriveWithWeight(gfx::Font::Weight::MEDIUM)})));
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(SK_ColorWHITE);
}

BEGIN_METADATA(SidebarItemAddedFeedbackBubble)
END_METADATA

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/bind.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/sidebar/bubble_border_with_arrow.h"
#include "brave/browser/ui/views/sidebar/sidebar_bubble_background.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_added_feedback_bubble.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
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

SidebarItemAddedFeedbackBubble::SidebarItemAddedFeedbackBubble(
    views::View* anchor_view,
    views::View* items_contents_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::LEFT_CENTER),
      animation_(base::TimeDelta::FromMilliseconds(kFadeoutDurationInMs),
                 60,
                 this) {
  // This bubble uses same color for all themes.
  constexpr SkColor kBubbleBackground = SkColorSetRGB(0x33, 0x9A, 0xF0);
  set_color(kBubbleBackground);
  // Give margin and arrow at there.
  set_margins(
      gfx::Insets(0, BubbleBorderWithArrow::kBubbleArrowBoundsWidth, 0, 0));
  set_title_margins(gfx::Insets());
  SetButtons(ui::DIALOG_BUTTON_NONE);

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
        FROM_HERE, base::TimeDelta::FromMilliseconds(kFadeoutStartTimeInMs),
        base::BindOnce(&gfx::Animation::Start, base::Unretained(&animation_)));
  }
}

std::unique_ptr<views::NonClientFrameView>
SidebarItemAddedFeedbackBubble::CreateNonClientFrameView(
    views::Widget* widget) {
  std::unique_ptr<views::BubbleFrameView> frame(
      new views::BubbleFrameView(gfx::Insets(), gfx::Insets(10, 18)));
  std::unique_ptr<BubbleBorderWithArrow> border =
      std::make_unique<BubbleBorderWithArrow>(arrow(), GetShadow(), color());
  constexpr int kFeedbackBubbleRadius = 6;
  border->SetCornerRadius(kFeedbackBubbleRadius);
  auto* border_ptr = border.get();
  frame->SetBubbleBorder(std::move(border));
  // Replace default background to draw arrow.
  frame->SetBackground(std::make_unique<SidebarBubbleBackground>(border_ptr));
  return frame;
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
      l10n_util::GetStringUTF16(IDS_SIDEBAR_ADDED_FEEDBACK_TITLE_1),
      views::Label::CustomFont(
          {views::Label::GetDefaultFontList().DeriveWithWeight(
              gfx::Font::Weight::SEMIBOLD)})));
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(SK_ColorWHITE);

  // 11pt (1 decreased from default(12)) and 500 weight.
  label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_SIDEBAR_ADDED_FEEDBACK_TITLE_2),
      views::Label::CustomFont(
          {views::Label::GetDefaultFontList()
               .DeriveWithSizeDelta(-1)
               .DeriveWithWeight(gfx::Font::Weight::MEDIUM)})));
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(SK_ColorWHITE);
}

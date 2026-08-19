/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/hovercard/fade_label_view.h"
#include "chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "content/public/common/url_constants.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_class_properties.h"

#define TabHoverCardBubbleView TabHoverCardBubbleView_ChromiumImpl
#include <chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc>
#undef TabHoverCardBubbleView

// BraveTabCardView ------------------------------------------------------------
class BraveTabCardView
    : public TabHoverCardBubbleView_ChromiumImpl::TabCardView {
  METADATA_HEADER(BraveTabCardView,
                  TabHoverCardBubbleView_ChromiumImpl::TabCardView)
 public:
  explicit BraveTabCardView(TabHoverCardBubbleView_ChromiumImpl* bubble_view)
      : TabCardView(bubble_view) {
    auto domain_label_index = GetIndexOf(domain_label_);
    CHECK(domain_label_index);

    container_label_ = AddChildViewAt(std::make_unique<views::LabelButton>(),
                                      *domain_label_index + 1);
    container_label_->SetProperty(
        views::kMarginsKey, gfx::Insets(kTextMargins).set_top_bottom(4, 12));

    // Fix vertical height
    container_label_->SetMaxSize({std::numeric_limits<int>::max(), 20});
    container_label_->SetMinSize({0, 20});

    container_label_->SetProperty(views::kCrossAxisAlignmentKey,
                                  views::LayoutAlignment::kStart);
    container_label_->SetImageLabelSpacing(3);
    container_label_->SetBorder(
        views::CreateEmptyBorder(gfx::Insets::TLBR(2, 4, 2, 7)));
  }
  ~BraveTabCardView() override = default;

  void UpdateContent(const HoverCardAnchorTarget* anchor_target) override {
    TabCardView::UpdateContent(anchor_target);

    auto container_card_data = anchor_target->GetContainerCardData();
    if (!container_card_data) {
      container_label_->SetVisible(false);
      container_label_->SetImageModel(views::Button::STATE_NORMAL, {});
      container_label_->SetText({});
      container_label_->SetBackground(nullptr);
      return;
    }

    container_label_->SetImageModel(views::Button::STATE_NORMAL,
                                    container_card_data->container_icon);
    container_label_->SetText(container_card_data->container_name);
    container_label_->SetBackground(views::CreateRoundedRectBackground(
        container_card_data->container_background_color,
        /*corner_radius=*/10));

    // Override bottom margin of a label right before the container_label_
    auto* label_before_container_label =
        domain_label_->GetVisible() ? domain_label_.get() : title_label_.get();
    auto* upstream_margins =
        label_before_container_label->GetProperty(views::kMarginsKey);
    CHECK(upstream_margins);
    label_before_container_label->SetProperty(
        views::kMarginsKey, gfx::Insets(*upstream_margins).set_bottom(0));

    container_label_->SetVisible(true);
  }

  views::LabelButton* container_label_for_testing() { return container_label_; }

 private:
  raw_ptr<views::LabelButton> container_label_ = nullptr;
};

BEGIN_METADATA(BraveTabCardView)
END_METADATA

// TabHoverCardBubbleView_ChromiumImpl -----------------------------------------
std::unique_ptr<TabHoverCardBubbleView_ChromiumImpl::TabCardView>
TabHoverCardBubbleView_ChromiumImpl::CreateTabCardView() {
  return std::make_unique<BraveTabCardView>(this);
}

bool TabHoverCardBubbleView_ChromiumImpl::HasThumbnailView() const {
  return tab_card_view_->thumbnail_view() != nullptr;
}

void TabHoverCardBubbleView_ChromiumImpl::BraveUpdateCardContent(
    const HoverCardAnchorTarget* anchor_target) {
  TabHoverCardBubbleView_ChromiumImpl::UpdateCardContent(anchor_target);
  // Replace chrome:// with brave://. Since this is purely in the UI we can
  // just do a sub-string replacement instead of parsing into GURL.
  auto domain = std::u16string(tab_card_view_->domain_label()->GetText());
  if (brave_utils::ReplaceChromeToBraveScheme(domain)) {
    tab_card_view_->domain_label()->SetData({domain, /*is_filename*/ false});
  }
}

// TabHoverCardBubbleView ------------------------------------------------------
void TabHoverCardBubbleView::UpdateCardContent(
    const HoverCardAnchorTarget* anchor_target) {
  BraveUpdateCardContent(anchor_target);
}

void TabHoverCardBubbleView::SetTargetTabImage(gfx::ImageSkia preview_image) {
  if (!HasThumbnailView()) {
    return;
  }
  TabHoverCardBubbleView_ChromiumImpl::SetTargetTabImage(preview_image);
}

void TabHoverCardBubbleView::SetPlaceholderImage() {
  if (!HasThumbnailView()) {
    return;
  }
  TabHoverCardBubbleView_ChromiumImpl::SetPlaceholderImage();
}

BEGIN_METADATA(TabHoverCardBubbleView)
END_METADATA

views::LabelButton* TabHoverCardBubbleView::GetContainerLabelForTesting() {
  return views::AsViewClass<BraveTabCardView>(
             GetTabCardViewForTesting())  // IN-TEST
      ->container_label_for_testing();    // IN-TEST
}

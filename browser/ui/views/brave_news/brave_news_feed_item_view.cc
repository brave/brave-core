// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_feed_item_view.h"

#include <memory>
#include <vector>

#include "brave/app/vector_icons/vector_icons.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"

BraveNewsFeedItemView::BraveNewsFeedItemView(
    BraveNewsTabHelper::FeedDetails details,
    content::WebContents* contents)
    : feed_details_(details), contents_(contents) {
  DCHECK(contents_);
  tab_helper_ = BraveNewsTabHelper::FromWebContents(contents);
  tab_helper_observation_.Observe(tab_helper_);

  auto* const layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

  auto* title = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(details.title)));
  title->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
  title->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kScaleToMaximum));
  title->SetMultiLine(false);
  title->SetMaximumWidthSingleLine(150);
  title->SetElideBehavior(gfx::ELIDE_TAIL);

  auto* spacer = AddChildView(std::make_unique<views::View>());
  spacer->SetPreferredSize(gfx::Size(8, 0));
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));

  subscribe_button_ = AddChildView(std::make_unique<views::MdTextButton>(
      base::BindRepeating(&BraveNewsFeedItemView::OnPressed,
                          base::Unretained(this)),
      u""));

  Update();
}

BraveNewsFeedItemView::~BraveNewsFeedItemView() = default;

void BraveNewsFeedItemView::Update() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto is_subscribed = tab_helper_->IsSubscribed(feed_details_);
  subscribe_button_->SetText(l10n_util::GetStringUTF16(
      is_subscribed ? IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_UNSUBSCRIBE
                    : IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_SUBSCRIBE));

  subscribe_button_->SetLoading(loading_);
  subscribe_button_->SetKind(is_subscribed ? views::MdTextButton::SECONDARY
                                           : views::MdTextButton::PRIMARY);
  subscribe_button_->SetIcon(is_subscribed ? &kBraveNewsUnfollowButtonIcon
                                           : &kBraveNewsFollowButtonIcon);
}

void BraveNewsFeedItemView::OnAvailableFeedsChanged(
    const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) {
  loading_ = false;
  Update();
}

void BraveNewsFeedItemView::OnPressed() {
  // Don't queue multiple toggles.
  if (loading_)
    return;

  tab_helper_->ToggleSubscription(feed_details_);
  loading_ = true;
  Update();
}

BEGIN_METADATA(BraveNewsFeedItemView, views::View)
END_METADATA

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_feed_item_view.h"

#include <memory>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"

namespace {
constexpr int kFollowButtonIconSize = 14;
}

BraveNewsFeedItemView::BraveNewsFeedItemView(const GURL& feed_url,
                                             content::WebContents* contents)
    : feed_url_(feed_url), contents_(contents) {
  DCHECK(contents_);
  tab_helper_ = BraveNewsTabHelper::FromWebContents(contents);
  tab_helper_observation_.Observe(tab_helper_);

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

  constexpr int kTitleWidth = 150;
  title_ = AddChildView(std::make_unique<views::Label>());
  title_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
  title_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kScaleToMaximum));
  title_->SetMultiLine(false);
  title_->SetMaximumWidthSingleLine(kTitleWidth);
  title_->SetPreferredSize(gfx::Size(kTitleWidth, 0));
  title_->SetElideBehavior(gfx::ELIDE_TAIL);

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

  auto title = tab_helper_->GetTitleForFeedUrl(feed_url_);

  // The only scenario where the title will be empty is when the feed doesn't
  // exist (most likely when if we tried to fetch the feed and it failed). In
  // that case, we should remove this row.
  if (title.empty() && this->parent()) {
    this->parent()->RemoveChildView(this);
    return;
  }

  title_->SetText(base::UTF8ToUTF16(title));
  auto is_subscribed = tab_helper_->IsSubscribed(feed_url_);
  subscribe_button_->SetText(l10n_util::GetStringUTF16(
      is_subscribed ? IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_UNSUBSCRIBE
                    : IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_SUBSCRIBE));

  subscribe_button_->SetLoading(loading_);
  subscribe_button_->SetStyle(is_subscribed ? ui::ButtonStyle::kDefault
                                            : ui::ButtonStyle::kProminent);
  subscribe_button_->SetIcon(
      is_subscribed ? &kLeoHeartFilledIcon : &kLeoHeartOutlineIcon,
      kFollowButtonIconSize);
}

void BraveNewsFeedItemView::OnPressed() {
  // Don't queue multiple toggles.
  if (loading_) {
    return;
  }

  tab_helper_->ToggleSubscription(feed_url_);
  loading_ = true;
  Update();
}

void BraveNewsFeedItemView::OnAvailableFeedsChanged(
    const std::vector<GURL>& feed_urls) {
  loading_ = false;
  Update();
}

BEGIN_METADATA(BraveNewsFeedItemView)
END_METADATA

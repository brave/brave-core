// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_feeds_container_view.h"
#include <memory>

#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/brave_news/brave_news_feed_item_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_class_properties.h"

BraveNewsFeedsContainerView::BraveNewsFeedsContainerView(
    content::WebContents* contents) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
  for (const auto& feed_item : tab_helper->GetAvailableFeeds()) {
    auto* child = AddChildView(
        std::make_unique<BraveNewsFeedItemView>(feed_item, contents));
    child->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(10, 0, 0, 0));
  }

  views::FlexLayout* const layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);
  layout->SetCollapseMargins(true);
}

BraveNewsFeedsContainerView::~BraveNewsFeedsContainerView() = default;

BEGIN_METADATA(BraveNewsFeedsContainerView, views::View)
END_METADATA

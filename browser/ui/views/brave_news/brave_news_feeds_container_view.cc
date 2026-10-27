// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_feeds_container_view.h"

#include <memory>
#include <vector>

#include "brave/browser/ui/views/brave_news/brave_news_feed_item_view.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_class_properties.h"

BraveNewsFeedsContainerView::BraveNewsFeedsContainerView(
    const std::vector<GURL>& available_feeds,
    content::WebContents* contents) {
  for (const auto& feed_item : available_feeds) {
    auto* child = AddChildView(
        std::make_unique<BraveNewsFeedItemView>(feed_item, contents));
    child->SetProperty(views::kMarginsKey, gfx::Insets::VH(12, 12));

    if (&feed_item != &available_feeds.back()) {
      auto* separator = AddChildView(std::make_unique<views::Separator>());
      separator->SetProperty(views::kMarginsKey, gfx::Insets::VH(0, 12));
      separator->SetOrientation(views::Separator::Orientation::kHorizontal);
    }
  }

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStretch)
      .SetCollapseMargins(false);

  constexpr float kCornerRadius = 12;
  SetBackground(views::CreateRoundedRectBackground(
      nala::kColorContainerBackground, kCornerRadius));
  SetBorder(views::CreateRoundedRectBorder(1, kCornerRadius,
                                           nala::kColorDividerSubtle));
}

BraveNewsFeedsContainerView::~BraveNewsFeedsContainerView() = default;

BEGIN_METADATA(BraveNewsFeedsContainerView)
END_METADATA

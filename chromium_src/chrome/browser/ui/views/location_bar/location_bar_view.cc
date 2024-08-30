/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/adapters.h"
#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"
#include "brave/browser/ui/views/page_action/brave_page_action_icon_container_view.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "ui/views/style/typography.h"

// |icon_left| - Padding between left border of location bar and first
//               decoration. Use element padding.
// |text_left| - Padding between omnibox view and last leading decoration.
//               If last decoration has label, it has sufficient padding inside.
//               If custom padding is provided(text_left is not null), respect
//               it. Otherwise, set our design value - 5px.
// Both value could be updated when |should_indent| is true.
#define BRAVE_LOCATION_BAR_VIEW_LAYOUT_LEADING_DECORATIONS         \
  icon_left = GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING);     \
  if (text_left == 0 && !location_icon_view_->ShouldShowLabel()) { \
    text_left = 5;                                                 \
  }

#define BRAVE_LOCATION_BAR_VIEW_LAYOUT_RIGHT_MOST_TRAILING_DECORATIONS \
  auto right_most_trailing_views = GetRightMostTrailingViews();        \
  for (auto* item : base::Reversed(right_most_trailing_views)) {       \
    add_trailing_decoration(item, /*intra_item_padding=*/0);           \
  }

#define BRAVE_LOCATION_BAR_VIEW_LAYOUT_LEFT_MOST_TRAILING_DECORATIONS \
  auto left_most_trailing_views = GetLeftMostTrailingViews();         \
  for (auto* item : base::Reversed(left_most_trailing_views)) {       \
    add_trailing_decoration(item, /*intra_item_padding=*/0);          \
  }

#define BRAVE_LOCATION_BAR_VIEW_LAYOUT_HANDLE_SEARCH_PROMOTION_BUTTON_VISIBILITY \
  }                                                                              \
  else if (GetSearchPromotionButton() && /* NOLINT */                            \
           GetSearchPromotionButton()->GetVisible()) {                           \
    leading_decorations.AddDecoration(vertical_padding, location_height,         \
                                      false, kLeadingDecorationMaxFraction,      \
                                      /*intra_item_padding=*/0, 0,               \
                                      GetSearchPromotionButton());

#define OmniboxViewViews BraveOmniboxViewViews
#define ChromeOmniboxClient BraveOmniboxClientImpl
#define PageActionIconContainerView BravePageActionIconContainerView
#define STYLE_BODY_4_EMPHASIS STYLE_PRIMARY

// We don't use different colors when the omnibox doesn't have focus but still
// contains in-progress user input.
#define kColorOmniboxResultsBackgroundHovered kColorLocationBarBackgroundHovered); \
  normal = color_provider->GetColor(kColorLocationBarBackground

#include "src/chrome/browser/ui/views/location_bar/location_bar_view.cc"
#undef STYLE_BODY_4_EMPHASIS
#undef kColorOmniboxResultsBackgroundHovered
#undef PageActionIconContainerView
#undef ChromeOmniboxClient
#undef OmniboxViewViews
#undef BRAVE_LOCATION_BAR_VIEW_LAYOUT_HANDLE_SEARCH_PROMOTION_BUTTON_VISIBILITY
#undef BRAVE_LOCATION_BAR_VIEW_LAYOUT_LEFT_MOST_TRAILING_DECORATIONS
#undef BRAVE_LOCATION_BAR_VIEW_LAYOUT_RIGHT_MOST_TRAILING_DECORATIONS
#undef BRAVE_LOCATION_BAR_VIEW_LAYOUT_LEADING_DECORATIONS

views::View* LocationBarView::GetSearchPromotionButton() const {
  return nullptr;
}

std::vector<views::View*> LocationBarView::GetRightMostTrailingViews() {
  return std::vector<views::View*>();
}

std::vector<views::View*> LocationBarView::GetLeftMostTrailingViews() {
  return std::vector<views::View*>();
}

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/groups/tab_group_editor_bubble_view.h"

#include "base/check.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/data_sharing/data_sharing_bubble_controller.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_utils.h"

namespace {

// If a "learn more" footer has been added to the bubble view, remove it and
// ensure that the view's interior margins are correct.
void MaybeRemoveFooter(TabGroupEditorBubbleView* bubble_view,
                       views::View* footer) {
  if (!footer) {
    return;
  }

  auto footer_holder = bubble_view->RemoveChildViewT(footer);

  auto* layout =
      static_cast<views::FlexLayout*>(bubble_view->GetLayoutManager());
  CHECK(layout);
  gfx::Insets margin = layout->interior_margin();
  margin.set_bottom(margin.top());
  layout->SetInteriorMargin(margin);
}

TabGroupEditorBubbleView* SetEditorBubbleArrowForBraveTabGroupHeader(
    TabGroupEditorBubbleView* bubble_view,
    views::View* anchor_view) {
  auto* group_header = views::AsViewClass<BraveTabGroupHeader>(anchor_view);
  if (group_header) {
    bubble_view->SetArrow(
        group_header->GetEditorBubbleArrow(bubble_view->arrow()));
  }
  return bubble_view;
}

}  // namespace

#define CreateBubble(bubble_view)                                            \
  CreateBubble(                                                              \
      SetEditorBubbleArrowForBraveTabGroupHeader(bubble_view, anchor_view)); \
  MaybeRemoveFooter(bubble_view, bubble_view->footer_.ExtractAsDangling());  \
  bubble_view->footer_ = nullptr

#define kUngroupRefreshOldIcon \
  kUngroupRefreshOldIcon, ui::kColorMenuIcon, kDefaultIconSize

#include <chrome/browser/ui/views/tabs/groups/tab_group_editor_bubble_view.cc>

#undef kUngroupRefreshOldIcon

#undef CreateBubble

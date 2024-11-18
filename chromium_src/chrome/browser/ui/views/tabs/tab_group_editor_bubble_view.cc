/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.h"

#include "chrome/browser/ui/views/data_sharing/data_sharing_bubble_controller.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "ui/views/layout/flex_layout.h"

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

}  // namespace

#define CreateBubble(bubble_view)                                           \
  CreateBubble(bubble_view);                                                \
  MaybeRemoveFooter(bubble_view, bubble_view->footer_.ExtractAsDangling()); \
  bubble_view->footer_ = nullptr

#include "src/chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.cc"

#undef CreateBubble

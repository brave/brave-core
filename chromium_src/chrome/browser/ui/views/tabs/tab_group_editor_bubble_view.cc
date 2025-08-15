/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/bubble_menu_item_factory.h"
#include "chrome/browser/ui/views/data_sharing/data_sharing_bubble_controller.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
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
  bubble_view->MaybeAddSuggestedTabsButton();                               \
  MaybeRemoveFooter(bubble_view, bubble_view->footer_.ExtractAsDangling()); \
  bubble_view->footer_ = nullptr

#define kUngroupRefreshIcon \
  kUngroupRefreshIcon, ui::kColorMenuIcon, kDefaultIconSize

#include <chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.cc>

#undef kUngroupRefreshIcon

#undef CreateBubble

std::unique_ptr<views::LabelButton>
TabGroupEditorBubbleView::BuildSuggestedTabInGroupButton() {
  return CreateMenuItem(
      TAB_GROUP_HEADER_CXMENU_ADD_SUGGESTED_TABS,
      l10n_util::GetStringUTF16(IDS_TAB_GROUP_HEADER_CXMENU_ADD_SUGGESTED_TABS),
      base::BindRepeating(&TabGroupEditorBubbleView::SuggestedTabsPressed,
                          base::Unretained(this)),
      ui::ImageModel::FromVectorIcon(kNewTabInGroupRefreshIcon,
                                     ui::kColorMenuIcon, 20));
}

void TabGroupEditorBubbleView::SuggestedTabsPressed() {
  // TODO: Implement AI-based tab suggestion logic
  // For now, just close the bubble
  GetWidget()->Close();
}

void TabGroupEditorBubbleView::MaybeAddSuggestedTabsButton() {
  // Find the position after "New tab in group" button and insert our button
  for (size_t i = 0; i < simple_menu_items_.size(); ++i) {
    if (simple_menu_items_[i]->GetID() ==
        TAB_GROUP_HEADER_CXMENU_NEW_TAB_IN_GROUP) {
      // Create our button
      auto suggested_button = BuildSuggestedTabInGroupButton();
      auto* suggested_button_ptr = suggested_button.get();

      // Find the NEW_TAB_IN_GROUP button in the view hierarchy and insert after
      // it
      views::LabelButton* new_tab_view = simple_menu_items_[i];
      auto view_index_opt = GetIndexOf(new_tab_view);
      if (!view_index_opt.has_value()) {
        AddChildView(std::move(suggested_button));  // Fallback to end
      } else {
        int view_index = static_cast<int>(view_index_opt.value());
        // Insert our button at the next position in the view hierarchy
        AddChildViewAt(std::move(suggested_button), view_index + 1);
      }

      // Also insert into simple_menu_items_ at the correct position
      simple_menu_items_.insert(simple_menu_items_.begin() + i + 1,
                                suggested_button_ptr);
      break;
    }
  }
}

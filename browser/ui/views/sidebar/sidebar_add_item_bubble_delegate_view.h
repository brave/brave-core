/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ADD_ITEM_BUBBLE_DELEGATE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ADD_ITEM_BUBBLE_DELEGATE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class BraveBrowser;

namespace sidebar {
struct SidebarItem;
}  // namespace sidebar

// TODO(simonhong): Need to apply UI design spec. Currently, this just works.
class SidebarAddItemBubbleDelegateView
    : public views::BubbleDialogDelegateView {
  METADATA_HEADER(SidebarAddItemBubbleDelegateView,
                  views::BubbleDialogDelegateView)
 public:

  static views::Widget* Create(BraveBrowser* browser, views::View* anchor_view);

  ~SidebarAddItemBubbleDelegateView() override;

  SidebarAddItemBubbleDelegateView(const SidebarAddItemBubbleDelegateView&) =
      delete;
  SidebarAddItemBubbleDelegateView& operator=(
      const SidebarAddItemBubbleDelegateView&) = delete;

 private:
  SidebarAddItemBubbleDelegateView(BraveBrowser* browser,
                                   views::View* anchor_view);

  void AddChildViews();

  // Passed |item| will be added to sidebar.
  void OnDefaultItemsButtonPressed(const sidebar::SidebarItem& item);
  void OnCurrentItemButtonPressed();
  void CloseOrReLayoutAfterAddingItem();

  raw_ptr<BraveBrowser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ADD_ITEM_BUBBLE_DELEGATE_VIEW_H_

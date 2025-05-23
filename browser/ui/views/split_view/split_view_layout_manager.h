/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LAYOUT_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LAYOUT_MANAGER_H_

#include "brave/browser/ui/views/split_view/split_view_separator_delegate.h"
#include "ui/views/controls/resize_area_delegate.h"
#include "ui/views/layout/layout_manager_base.h"
#include "ui/views/layout/proposed_layout.h"

class SplitViewSeparator;

class SplitViewLayoutManager : public views::LayoutManagerBase,
                               public views::ResizeAreaDelegate,
                               public SplitViewSeparatorDelegate {
 public:
  // Spacing between |contents_web_view_| and |secondary_contents_web_view_|.
  static constexpr auto kSpacingBetweenContentsWebViews = 4;

  SplitViewLayoutManager(views::View* contents_container,
                         views::View* secondary_contents_container,
                         SplitViewSeparator* split_view_separator);
  ~SplitViewLayoutManager() override;

  int split_view_size_delta() const { return split_view_size_delta_; }
  void set_split_view_size_delta(int delta) { split_view_size_delta_ = delta; }

  // When tile's second tab is the active web contents, we need to show the
  // tab after the first tab.
  void show_main_web_contents_at_tail(bool tail) {
    show_main_web_contents_at_tail_ = tail;
  }

  // SplitViewSeparatorDelegate:
  void OnDoubleClicked() override;

  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override;

 protected:
  // views::LayoutManagerBase overrides:
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;

 private:
  friend class SplitViewLayoutManagerUnitTest;

  raw_ptr<views::View> contents_container_ = nullptr;
  raw_ptr<views::View> secondary_contents_container_ = nullptr;
  raw_ptr<SplitViewSeparator> split_view_separator_ = nullptr;

  int split_view_size_delta_ = 0;
  int ongoing_split_view_size_delta_ = 0;

  bool show_main_web_contents_at_tail_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LAYOUT_MANAGER_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_VIEW_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/views/frame/contents_container_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#endif

namespace gfx {
class RoundedCornersF;
}  // namespace gfx

class BraveContentsContainerView :
#if BUILDFLAG(ENABLE_SPEEDREADER)
    public ReaderModeToolbarView::Delegate,
#endif
    public ContentsContainerView {
  METADATA_HEADER(BraveContentsContainerView, ContentsContainerView)
 public:
  static constexpr int kBorderThickness = 2;

  static BraveContentsContainerView* From(ContentsContainerView* view);

  explicit BraveContentsContainerView(BrowserView* browser_view);
  ~BraveContentsContainerView() override;

  // ContentsContainerView:
  void UpdateBorderAndOverlay(bool is_in_split,
                              bool is_active,
                              bool is_highlighted) override;
  void UpdateBorderRoundedCorners() override;
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;
  void ChildVisibilityChanged(views::View* child) override;

#if BUILDFLAG(ENABLE_SPEEDREADER)
  // ReaderModeToolbarView::Delegate:
  void OnReaderModeToolbarActivate(ReaderModeToolbarView* toolbar) override;

  ReaderModeToolbarView* reader_mode_toolbar() { return reader_mode_toolbar_; }
#endif

 private:
  FRIEND_TEST_ALL_PREFIXES(SideBySideEnabledBrowserTest,
                           BraveMultiContentsViewTest);

  gfx::RoundedCornersF GetCornerRadius(bool for_border) const;

  raw_ref<BrowserView> browser_view_;

#if BUILDFLAG(ENABLE_SPEEDREADER)
  raw_ptr<ReaderModeToolbarView> reader_mode_toolbar_ = nullptr;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_VIEW_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_VIEW_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/frame/contents_container_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveContentsContainerView : public ContentsContainerView {
  METADATA_HEADER(BraveContentsContainerView, ContentsContainerView)
 public:
  explicit BraveContentsContainerView(BrowserView* browser_view);
  ~BraveContentsContainerView() override;

  // ContentsContainerView:
  void UpdateBorderAndOverlay(bool is_in_split,
                              bool is_active,
                              bool show_scrim) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SideBySideEnabledBrowserTest,
                           BraveMultiContentsViewTest);

  static constexpr int kBorderThickness = 2;

  float GetCornerRadius(bool for_border) const;

  raw_ref<BrowserView> browser_view_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_VIEW_H_

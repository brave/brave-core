/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_

#include <vector>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveMultiContentsView : public MultiContentsView {
  METADATA_HEADER(BraveMultiContentsView, MultiContentsView)

 public:
  static constexpr int kBorderThickness = 2;

  using MultiContentsView::MultiContentsView;
  ~BraveMultiContentsView() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SideBySideEnabledBrowserTest,
                           BraveMultiContentsViewTest);

  // MultiContentsView:
  void UpdateContentsBorder() override;
  void Layout(PassKey) override;

  float GetCornerRadius() const;

  std::vector<ContentsContainerView*> contents_container_views_for_testing()
      const {
    return contents_container_views_;
  }
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_

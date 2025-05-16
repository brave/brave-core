/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_

#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveMultiContentsView : public MultiContentsView {
  METADATA_HEADER(BraveMultiContentsView, MultiContentsView)

 public:
  using MultiContentsView::MultiContentsView;
  ~BraveMultiContentsView() override;

 private:
  // MultiContentsView:
  void UpdateContentsBorder() override;
  void Layout(PassKey) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_

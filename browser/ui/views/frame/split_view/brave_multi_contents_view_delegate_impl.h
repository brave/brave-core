/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_DELEGATE_IMPL_H_

#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"

class TabStripModel;

class BraveMultiContentsViewDelegateImpl
    : public MultiContentsViewDelegateImpl {
 public:
  explicit BraveMultiContentsViewDelegateImpl(Browser& browser);
  ~BraveMultiContentsViewDelegateImpl() override;

  // MultiContentsViewDelegateImpl:
  void ResizeWebContents(double ratio, bool done_resizing) override;

 private:
  const raw_ref<TabStripModel> tab_strip_model_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_DELEGATE_IMPL_H_

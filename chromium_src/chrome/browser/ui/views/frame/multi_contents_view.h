/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_H_

#define UpdateSplitRatio(...)       \
  UpdateSplitRatio(__VA_ARGS__);    \
  virtual void ResetResizeArea() {} \
  friend class BraveMultiContentsView

#define UpdateContentsBorderAndOverlay virtual UpdateContentsBorderAndOverlay
#define OnWebContentsFocused virtual OnWebContentsFocused
#define ExecuteOnEachVisibleContentsView \
  virtual ExecuteOnEachVisibleContentsView
#define GetActiveContentsContainerView     \
  GetActiveContentsContainerView_UnUsed(); \
  virtual ContentsContainerView* GetActiveContentsContainerView

#define GetActiveContentsView     \
  GetActiveContentsView_UnUsed(); \
  virtual ContentsWebView* GetActiveContentsView

#define GetContentsContainerViewFor     \
  GetContentsContainerViewFor_UnUsed(); \
  virtual ContentsContainerView* GetContentsContainerViewFor

#include <chrome/browser/ui/views/frame/multi_contents_view.h>  // IWYU pragma: export

#undef GetContentsContainerViewFor
#undef GetActiveContentsView
#undef GetActiveContentsContainerView
#undef ExecuteOnEachVisibleContentsView
#undef OnWebContentsFocused
#undef UpdateContentsBorderAndOverlay
#undef UpdateSplitRatio

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_H_

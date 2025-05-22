/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_H_

namespace views {
class ResizeArea;
}  // namespace views

#define UpdateContentsBorderAndOverlay \
  UnUsed() {}                          \
  friend class BraveMultiContentsView; \
  virtual void UpdateContentsBorderAndOverlay

#define SetActiveIndex virtual SetActiveIndex

// Changed to base class as we want to point to our views::ResizeArea subclass.
#define MultiContentsResizeArea views::ResizeArea

#include "src/chrome/browser/ui/views/frame/multi_contents_view.h"  // IWYU pragma: export

#undef MultiContentsResizeArea
#undef SetActiveIndex
#undef UpdateContentsBorderAndOverlay

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_H_

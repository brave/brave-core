/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_

#define BRAVE_TRANSLATE_BUBBLE_VIEW_H_                                     \
 private:                                                                  \
  friend class BraveTranslateBubbleView;                                   \
  friend class BraveTranslateBubbleViewTest;                               \
  FRIEND_TEST_ALL_PREFIXES(BraveTranslateBubbleViewTest,                   \
                           BraveBeforeTranslateView);                      \
  FRIEND_TEST_ALL_PREFIXES(BraveTranslateBubbleViewTest, TranslateButton); \
  FRIEND_TEST_ALL_PREFIXES(BraveTranslateBubbleViewTest, CancelButton);    \
                                                                           \
  template <typename... Args>                                              \
  static TranslateBubbleView* MakeTranslateBubbleView(Args&&... args);     \
                                                                           \
 public:                                                                   \
  virtual int GetTitleBeforeTranslateTitle();
 #define MAKE_BRAVE_TRANSLATE_BUBBLE_VIEW MakeTranslateBubbleView

class BraveTranslateBubbleView;
#include "../../../../../../../chrome/browser/ui/views/translate/translate_bubble_view.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_

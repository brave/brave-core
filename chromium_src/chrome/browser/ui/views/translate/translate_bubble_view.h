/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_

#define TranslateBubbleView TranslateBubbleView_ChromiumImpl
#define CreateTranslateIcon virtual CreateTranslateIcon
#include "src/chrome/browser/ui/views/translate/translate_bubble_view.h"
#undef CreateTranslateIcon
#undef TranslateBubbleView

class TranslateBubbleView : public TranslateBubbleView_ChromiumImpl {
 public:
  using TranslateBubbleView_ChromiumImpl::TranslateBubbleView_ChromiumImpl;

  std::unique_ptr<views::ImageView> CreateTranslateIcon() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_

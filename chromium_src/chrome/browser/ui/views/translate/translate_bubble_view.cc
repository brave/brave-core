/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/translate/translate_bubble_view.h"

#include "ui/views/controls/image_view.h"

std::unique_ptr<views::ImageView> TranslateBubbleView::CreateTranslateIcon() {
  return std::make_unique<views::ImageView>();
}

#define TranslateBubbleView TranslateBubbleView_ChromiumImpl
#include "src/chrome/browser/ui/views/translate/translate_bubble_view.cc"
#undef TranslateBubbleView

BEGIN_METADATA(TranslateBubbleView, TranslateBubbleView_ChromiumImpl)
END_METADATA

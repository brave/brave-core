/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"

#include "brave/components/translate/core/common/brave_translate_features.h"
#include "brave/components/translate/core/common/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"
#include "ui/views/controls/image_view.h"

int TranslateBubbleView_ChromiumImpl::GetTitleBeforeTranslateTitle() {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  return IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE;
#else
  return IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE;
#endif
}

// static
template <typename... Args>
TranslateBubbleView_ChromiumImpl*
TranslateBubbleView_ChromiumImpl::MakeTranslateBubbleView(Args&&... args) {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  if (translate::IsTranslateExtensionAvailable()) {
    return new BraveTranslateBubbleView(std::forward<Args>(args)...);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  return new TranslateBubbleView(std::forward<Args>(args)...);
}

std::unique_ptr<views::ImageView> TranslateBubbleView::CreateTranslateIcon() {
  return std::make_unique<views::ImageView>();
}

#define ORIGINAL_IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO) || \
    BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  GetTitleBeforeTranslateTitle()
#endif

#define MAKE_BRAVE_TRANSLATE_BUBBLE_VIEW MakeTranslateBubbleView
#define TranslateBubbleView TranslateBubbleView_ChromiumImpl
#include "src/chrome/browser/ui/views/translate/translate_bubble_view.cc"
#undef TranslateBubbleView
#undef MAKE_BRAVE_TRANSLATE_BUBBLE_VIEW
#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  ORIGINAL_IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#undef ORIGINAL_IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/translate/translate_bubble_controller.h"
#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"
#include "brave/components/translate/core/common/brave_translate_features.h"
#include "brave/components/translate/core/common/buildflags.h"

template <typename... Args>
std::unique_ptr<TranslateBubbleView> MakeTranslateBubbleView(Args&&... args) {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  if (translate::IsTranslateExtensionAvailable()) {
    return std::make_unique<BraveTranslateBubbleView>(
        std::forward<Args>(args)...);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  return std::make_unique<TranslateBubbleView>(std::forward<Args>(args)...);
}

#define MAKE_BRAVE_TRANSLATE_BUBBLE_VIEW MakeTranslateBubbleView

#include "src/chrome/browser/ui/views/translate/translate_bubble_controller.cc"
#undef MAKE_BRAVE_TRANSLATE_BUBBLE_VIEW

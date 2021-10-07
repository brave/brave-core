/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"
#include "brave/components/translate/core/browser/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#elif BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_INSTALL_TITLE
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
#define BRAVE_TRANSLATE_BUBBLE_VIEW_ BraveTranslateBubbleView
#else
#define BRAVE_TRANSLATE_BUBBLE_VIEW_ TranslateBubbleView
#endif

#include "../../../../../../../chrome/browser/ui/views/translate/translate_bubble_view.cc"

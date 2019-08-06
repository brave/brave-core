/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"


#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE)
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#else
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_INSTALL_TITLE
#endif


#include "../../../../../../chrome/browser/ui/views/translate/translate_bubble_view.cc" // NOLINT

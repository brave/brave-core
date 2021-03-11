/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_FONT_FALLBACK_LIST                                      \
  if (!GetFontSelector()->AllowFontFamily(curr_family->FamilyName())) \
    result = nullptr;

#include "src/third_party/blink/renderer/platform/fonts/font_fallback_list.cc"

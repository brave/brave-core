/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// We must include these explicitly before redefining method names that they
// also use.
#include "third_party/blink/renderer/platform/fonts/font_fallback_list.h"
#include "third_party/blink/renderer/platform/fonts/font_selector.h"

#define IsPlatformFamilyMatchAvailable                                       \
  AllowFontFamily(const AtomicString& family_name) override { return true; } \
  bool IsPlatformFamilyMatchAvailable

#include "src/third_party/blink/renderer/platform/testing/font_test_helpers.cc"

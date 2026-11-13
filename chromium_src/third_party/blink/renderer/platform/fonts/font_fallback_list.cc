/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/fonts/font_fallback_list.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "third_party/blink/renderer/platform/fonts/font_family.h"
#include "third_party/blink/renderer/platform/fonts/font_selector.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace brave {

namespace {

AllowFontFamilyCallback* GetAllowFontFamilyCallback() {
  static base::NoDestructor<AllowFontFamilyCallback> callback;
  return callback.get();
}

// Only asked of system fonts the font selector (CSS or offscreen) didn't
// match, so that web fonts are always allowed.
bool IsFontFamilyAllowed(const blink::FontSelector& font_selector,
                         const blink::FontFamily& family) {
  if (family.FamilyIsGeneric()) {
    return true;
  }
  return GetAllowFontFamilyCallback()->Run(font_selector.GetExecutionContext(),
                                           family.FamilyName());
}

}  // namespace

void RegisterAllowFontFamilyCallback(AllowFontFamilyCallback callback) {
  DCHECK(GetAllowFontFamilyCallback()->is_null());
  *GetAllowFontFamilyCallback() = std::move(callback);
}

}  // namespace brave

#include <third_party/blink/renderer/platform/fonts/font_fallback_list.cc>

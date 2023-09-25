/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/fonts/font_fallback_list.h"

#include "base/no_destructor.h"
#include "third_party/blink/renderer/platform/fonts/font_selector.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace brave {

namespace {

AllowFontFamilyCallback* GetAllowFontFamilyCallback() {
  static base::NoDestructor<AllowFontFamilyCallback> callback;
  return callback.get();
}

}  // namespace

void RegisterAllowFontFamilyCallback(AllowFontFamilyCallback callback) {
  DCHECK(GetAllowFontFamilyCallback()->is_null());
  *GetAllowFontFamilyCallback() = std::move(callback);
}

}  // namespace brave

// This only runs if the relevant font selector (CSS or offscreen)
// does NOT find a matching font, because we want to allow web fonts
// unconditionally.
#define BRAVE_GET_FONT_DATA                         \
  if ((!curr_family->FamilyIsGeneric()) &&          \
      brave::GetAllowFontFamilyCallback() &&        \
      !brave::GetAllowFontFamilyCallback()->Run(    \
          GetFontSelector()->GetExecutionContext(), \
          curr_family->FamilyName()))               \
    result = nullptr;

#include "src/third_party/blink/renderer/platform/fonts/font_fallback_list.cc"
#undef BRAVE_GET_FONT_DATA

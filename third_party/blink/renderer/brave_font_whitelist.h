/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FONT_WHITELIST_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FONT_WHITELIST_H_

#include <string>

#include "base/containers/flat_set.h"
#include "base/strings/string_piece.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave {

BLINK_EXPORT bool CanRestrictFontFamiliesOnThisPlatform();
BLINK_EXPORT const base::flat_set<base::StringPiece>& GetAllowedFontFamilies();

BLINK_EXPORT bool AllowFontByFamilyName(const AtomicString& family_name);

// This takes a 2-character language code.
BLINK_EXPORT const base::flat_set<base::StringPiece>&
GetAdditionalAllowedFontFamiliesByLocale(WTF::String locale);

// This takes a 2-character language code.
BLINK_EXPORT bool AllowFontByFamilyNameAndLocale(
    const AtomicString& family_name,
    WTF::String locale);

BLINK_EXPORT void set_allowed_font_families_for_testing(
    bool can_restrict_fonts,
    const base::flat_set<base::StringPiece>& allowed_font_families);

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FONT_WHITELIST_H_

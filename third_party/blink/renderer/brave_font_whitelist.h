/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FONT_WHITELIST_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FONT_WHITELIST_H_

#include <string>
#include <string_view>

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave {

BLINK_EXPORT bool AllowFontByFamilyName(const AtomicString& family_name,
                                        WTF::String default_language);

BLINK_EXPORT bool IsFontAllowedForFarbling(const AtomicString& family_name);

// Public for testing but other callers should call
// AllowFontByFamilyName instead.
BLINK_EXPORT base::span<const std::string_view>
GetAdditionalFontWhitelistByLocale(WTF::String locale_language);

// Testing-only functions

BLINK_EXPORT size_t GetFontWhitelistSizeForTesting();

BLINK_EXPORT void SetSimulateEmptyFontWhitelistForTesting(bool enable);

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FONT_WHITELIST_H_

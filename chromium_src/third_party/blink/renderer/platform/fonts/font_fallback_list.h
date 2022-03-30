/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/callback.h"
#include "src/third_party/blink/renderer/platform/fonts/font_fallback_list.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {
class ExecutionContext;
}

namespace brave {

typedef base::RepeatingCallback<bool(blink::ExecutionContext*,
                                     const WTF::AtomicString&)>
    AllowFontFamilyCallback;

PLATFORM_EXPORT void RegisterAllowFontFamilyCallback(
    AllowFontFamilyCallback callback);

}  // namespace brave

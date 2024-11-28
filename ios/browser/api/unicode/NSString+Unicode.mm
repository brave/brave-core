// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/unicode/NSString+Unicode.h"
#include "third_party/icu/source/common/unicode/ubidi.h"

@implementation NSString (Unicode)

- (ICUUBiDiDirection)bidiDirection {
  auto utf16_string = base::SysNSStringToUTF16(self);
  if (!utf16_string.empty()) {
    UBiDi* bidi = ubidi_open();
    CHECK(bidi);

    UErrorCode status = U_ZERO_ERROR;
    ubidi_setPara(bidi, &utf16_string[0],
                  static_cast<std::int32_t>(utf16_string.size()),
                  UBIDI_DEFAULT_LTR, nullptr, &status);
    if (U_FAILURE(status)) {
      ubidi_close(bidi);
      // Error as this function can never return neutral
      return ICUUBiDiDirectionLeftToRight;
    }

    UBiDiDirection direction = ubidi_getDirection(bidi);
    ubidi_close(bidi);
    return static_cast<ICUUBiDiDirection>(direction);
  }

  return ICUUBiDiDirectionLeftToRight;
}

- (ICUUBiDiDirection)bidiBaseDirection {
  auto utf16_string = base::SysNSStringToUTF16(self);
  if (!utf16_string.empty()) {
    return static_cast<ICUUBiDiDirection>(ubidi_getBaseDirection(
        &utf16_string[0], static_cast<std::int32_t>(utf16_string.size())));
  }

  return ICUUBiDiDirectionNeutral;
}

@end

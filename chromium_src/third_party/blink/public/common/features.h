/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_FEATURES_H_

#include "src/third_party/blink/public/common/features.h"

namespace blink {
namespace features {

BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kAllowCertainClientHints);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kFileSystemAccessAPI);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kNavigatorConnectionAttribute);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kPartitionBlinkMemoryCache);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kRestrictWebSocketsPool);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveBlockScreenFingerprinting);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveTorWindowsHttpsOnly);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveRoundTimeStamps);

}  // namespace features
}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_FEATURES_H_

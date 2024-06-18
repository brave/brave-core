/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_FEATURES_H_

#include "src/third_party/blink/public/common/features.h"  // IWYU pragma: export

namespace blink {
namespace features {

BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kFileSystemAccessAPI);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveWebBluetoothAPI);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kNavigatorConnectionAttribute);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kPartitionBlinkMemoryCache);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kRestrictWebSocketsPool);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveBlockScreenFingerprinting);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveGlobalPrivacyControl);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kBraveRoundTimeStamps);
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kRestrictEventSourcePool);

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
BLINK_COMMON_EXPORT BASE_DECLARE_FEATURE(kMiddleButtonClickAutoscroll);
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

// Chromium used this function to control Prerender2 feature, but then the
// feature was permanently enabled and the function was removed. We still want
// to keep the Prerender2 functionality disabled, so putting back the function
// to use in various places where Prerender2 needs to be turned off.
BLINK_COMMON_EXPORT bool IsPrerender2Enabled();

}  // namespace features
}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_FEATURES_H_

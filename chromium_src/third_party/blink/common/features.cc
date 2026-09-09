/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <third_party/blink/common/features.cc>

namespace blink::features {

BASE_FEATURE(kFileSystemAccessAPI, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWebBluetoothAPI, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNavigatorConnectionAttribute, base::FEATURE_DISABLED_BY_DEFAULT);

// Enable blink::MemoryCache partitioning for non SameSite requests.
BASE_FEATURE(kPartitionBlinkMemoryCache, base::FEATURE_ENABLED_BY_DEFAULT);

// Enable WebSockets connection pool limit per eTLD+1 for each renderer.
BASE_FEATURE(kRestrictWebSocketsPool, base::FEATURE_ENABLED_BY_DEFAULT);

// Enables protection against fingerprinting on screen dimensions.
BASE_FEATURE(kBraveBlockScreenFingerprinting,
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

// Enables protection against fingerprinting via high-resolution time stamps.
BASE_FEATURE(kBraveRoundTimeStamps, base::FEATURE_DISABLED_BY_DEFAULT);

// Enables protection against WebGL debug info fingerprinting on balanced
// farbling.
BASE_FEATURE(kWebGLBalancedFingerprintingProtection,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables the Global Privacy Control header and navigator APIs.
BASE_FEATURE(kBraveGlobalPrivacyControl, base::FEATURE_ENABLED_BY_DEFAULT);

// Enable EventSource connection pool limit per eTLD+1.
BASE_FEATURE(kRestrictEventSourcePool,
#if BUILDFLAG(IS_ANDROID)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
BASE_FEATURE(kMiddleButtonClickAutoscroll,
             "MiddelButtonClickAutoscroll",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
BASE_FEATURE(kForceContextMenuOnShiftRightClick,
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

bool IsPrerender2Enabled() {
  return false;
}

}  // namespace blink::features

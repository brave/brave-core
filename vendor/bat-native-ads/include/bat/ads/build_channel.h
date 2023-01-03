/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_BUILD_CHANNEL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_BUILD_CHANNEL_H_

#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads {

// Returns the build channel. |name| containing the build channel name.
// |is_release| containing |true| if a release build otherwise |false|.
mojom::BuildChannelInfo& BuildChannel();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_BUILD_CHANNEL_H_

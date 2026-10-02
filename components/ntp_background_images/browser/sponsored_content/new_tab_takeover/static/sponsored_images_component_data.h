/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_STATIC_SPONSORED_IMAGES_COMPONENT_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_STATIC_SPONSORED_IMAGES_COMPONENT_DATA_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "crypto/sha2.h"

namespace ntp_background_images {

struct SponsoredImagesComponentInfo {
  std::array<uint8_t, crypto::kSHA256Length> public_key_sha256;
  std::string_view id;
};

// Returns sponsored images component info for the given country code (ISO
// 3166-1 alpha-2). If no component is available for the specified country,
// returns `std::nullopt`.
std::optional<SponsoredImagesComponentInfo> GetSponsoredImagesComponent(
    std::string_view country_code);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_STATIC_SPONSORED_IMAGES_COMPONENT_DATA_H_

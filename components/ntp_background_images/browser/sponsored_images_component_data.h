/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_IMAGES_COMPONENT_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_IMAGES_COMPONENT_DATA_H_

#include <optional>
#include <string_view>

namespace ntp_background_images {

struct SponsoredImagesComponentInfo {
  std::string_view public_key_base64;
  std::string_view id;
};

// Returns sponsored images component info for the given country code (ISO
// 3166-1 alpha-2). If no component is available for the specified country,
// returns `std::nullopt`.
std::optional<SponsoredImagesComponentInfo> GetSponsoredImagesComponent(
    std::string_view country_code);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_IMAGES_COMPONENT_DATA_H_

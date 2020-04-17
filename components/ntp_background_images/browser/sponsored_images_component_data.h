/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_IMAGES_COMPONENT_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_IMAGES_COMPONENT_DATA_H_

#include <string>

#include "base/optional.h"

namespace ntp_background_images {

struct SponsoredImagesComponentData {
  std::string region;
  std::string component_base64_public_key;
  std::string component_id;
};

base::Optional<SponsoredImagesComponentData> GetSponsoredImagesComponentData(
    const std::string& region);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_IMAGES_COMPONENT_DATA_H_

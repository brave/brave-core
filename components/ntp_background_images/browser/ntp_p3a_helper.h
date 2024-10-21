/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_P3A_HELPER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_P3A_HELPER_H_

#include <string>

#include "url/gurl.h"

namespace ntp_background_images {

class NTPP3AHelper {
 public:
  virtual ~NTPP3AHelper() {}

  virtual void RecordView(const std::string& creative_instance_id,
                          const std::string& campaign_id) = 0;

  virtual void RecordClickAndMaybeLand(
      const std::string& creative_instance_id) = 0;

  virtual void SetLastTabURL(const GURL& url) = 0;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_P3A_HELPER_H_

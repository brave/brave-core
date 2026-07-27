/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_SITES_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_SITES_DATA_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "url/gurl.h"

namespace base {
class DictValue;
}  // namespace base

namespace ntp_background_images {

struct NTPSponsoredSite {
  std::string relative_image_url_spec;
  std::string title;
  std::string ad_disclosure;
  GURL target_url;
};

struct NTPSponsoredSitesData {
  NTPSponsoredSitesData();
  NTPSponsoredSitesData(const base::DictValue& dict,
                        const base::FilePath& installed_dir,
                        const std::string& url_prefix);

  NTPSponsoredSitesData(const NTPSponsoredSitesData&);
  NTPSponsoredSitesData& operator=(const NTPSponsoredSitesData&);

  NTPSponsoredSitesData(NTPSponsoredSitesData&&) noexcept;
  NTPSponsoredSitesData& operator=(NTPSponsoredSitesData&&) noexcept;

  ~NTPSponsoredSitesData();

  [[nodiscard]] bool IsValid() const;

  int schema_version = 0;
  std::vector<NTPSponsoredSite> sites;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_SITES_DATA_H_

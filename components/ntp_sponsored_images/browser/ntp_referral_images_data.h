/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGES_DATA_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGES_DATA_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"

namespace ntp_sponsored_images {

struct TopSite {
  TopSite();
  TopSite(const TopSite& data);
  TopSite& operator=(const TopSite& data);
  TopSite(TopSite&& data);
  ~TopSite();

  std::string icon_image_url() const;

  std::string name;
  std::string destination_url;
  base::FilePath icon_image_file;
  std::string url_prefix;
};

struct NTPReferralImagesData {
  NTPReferralImagesData();
  // Parsing data.json in referral images component.
  NTPReferralImagesData(const std::string& data_json,
                        const base::FilePath& base_dir);
  NTPReferralImagesData(const NTPReferralImagesData& data);
  NTPReferralImagesData& operator=(const NTPReferralImagesData& data);
  NTPReferralImagesData(NTPReferralImagesData&& data);
  ~NTPReferralImagesData();

  bool IsValid() const;
  // Generate Value with background image at |index|.
  base::Value GetValueAt(size_t index);

  std::string logo_image_url() const;
  std::vector<std::string> wallpaper_image_urls() const;

  base::FilePath logo_image_file;
  std::string logo_alt_text;
  std::string logo_company_name;
  std::string logo_destination_url;
  std::vector<base::FilePath> wallpaper_image_files;
  std::vector<TopSite> top_sites;
  std::string url_prefix;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGES_DATA_H_

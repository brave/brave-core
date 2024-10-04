/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace brave_ads {
struct NewTabPageAdInfo;
}  // namespace brave_ads

namespace ntp_background_images {

struct TopSite {
  std::string name;
  std::string destination_url;
  std::string background_color;
  std::string image_path;
  base::FilePath image_file;

  TopSite();
  // For unit test.
  TopSite(const std::string& name,
          const std::string& destination_url,
          const std::string& image_path,
          const base::FilePath& image_file);
  TopSite(const TopSite& data);
  TopSite& operator=(const TopSite& data);
  ~TopSite();

  bool IsValid() const;
};

struct Logo {
  base::FilePath image_file;
  std::string image_url;
  std::string alt_text;
  std::string destination_url;
  std::string company_name;

  Logo();
  Logo(const Logo&);
  ~Logo();
};

struct SponsoredBackground {
  base::FilePath image_file;
  gfx::Point focal_point;
  std::string background_color;

  std::string creative_instance_id;

  Logo logo;
  std::optional<gfx::Rect> viewbox;

  SponsoredBackground();
  // For unit test.
  SponsoredBackground(const base::FilePath& image_file_path,
                      const gfx::Point& point,
                      const Logo& test_logo,
                      const std::string& creative_instance_id);
  SponsoredBackground(const SponsoredBackground&);

  ~SponsoredBackground();
};

struct Campaign {
  Campaign();
  ~Campaign();
  Campaign(const Campaign&);
  Campaign& operator=(const Campaign&);

  bool IsValid() const;

  std::string campaign_id;
  std::vector<SponsoredBackground> backgrounds;
};

// For SI, campaign list can have multiple items.
// For SR, campaign list has only one item.
struct NTPSponsoredImagesData {
  NTPSponsoredImagesData();
  NTPSponsoredImagesData(const std::string& json_string,
                         const base::FilePath& installed_dir);
  NTPSponsoredImagesData(const NTPSponsoredImagesData& data);
  NTPSponsoredImagesData& operator=(const NTPSponsoredImagesData& data);
  ~NTPSponsoredImagesData();

  bool IsValid() const;

  void ParseCampaignsList(const base::Value::List& campaigns_value,
                          const base::FilePath& installed_dir);

  // Parse common properties for SI & SR.
  Campaign GetCampaignFromValue(const base::Value::Dict& value,
                                const base::FilePath& installed_dir);
  void ParseSRProperties(const base::Value::Dict& value,
                         const base::FilePath& installed_dir);

  std::optional<base::Value::Dict> GetBackgroundAt(size_t campaign_index,
                                                   size_t background_index);
  std::optional<base::Value::Dict> GetBackgroundFromAdInfo(
      const brave_ads::NewTabPageAdInfo& ad_info);

  bool IsSuperReferral() const;
  void PrintCampaignsParsingResult() const;

  bool AdInfoMatchesSponsoredImage(const brave_ads::NewTabPageAdInfo& ad_info,
                                   size_t campaign_index,
                                   size_t background_index) const;

  std::string url_prefix;

  std::vector<Campaign> campaigns;

  // SR only properties.
  std::string theme_name;
  std::vector<TopSite> top_sites;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_

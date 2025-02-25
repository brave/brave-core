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
#include "brave/components/brave_ads/core/public/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace brave_ads {
struct NewTabPageAdInfo;
}  // namespace brave_ads

namespace ntp_background_images {

inline constexpr char kImageWallpaperType[] = "image";
inline constexpr char kRichMediaWallpaperType[] = "richMedia";

struct TopSite {
  TopSite();

  // For unit test.
  TopSite(const std::string& name,
          const std::string& destination_url,
          const std::string& image_path,
          const base::FilePath& image_file);

  TopSite(const TopSite&);
  TopSite& operator=(const TopSite&);

  TopSite(TopSite&&) noexcept;
  TopSite& operator=(TopSite&&) noexcept;

  ~TopSite();

  [[nodiscard]] bool IsValid() const;

  std::string name;
  std::string destination_url;
  std::string background_color;
  std::string image_path;
  base::FilePath image_file;
};

struct Logo {
  Logo();

  Logo(const Logo&);
  Logo& operator=(const Logo&);

  Logo(Logo&&) noexcept;
  Logo& operator=(Logo&&) noexcept;

  ~Logo();

  base::FilePath image_file;
  std::string image_url;
  std::string alt_text;
  std::string destination_url;
  std::string company_name;
};

enum class WallpaperType { kImage, kRichMedia };

struct Creative {
  Creative();

  // For unit test.
  Creative(const base::FilePath& file_path,
           const gfx::Point& point,
           const Logo& test_logo,
           const std::string& creative_instance_id);

  Creative(const Creative&);
  Creative& operator=(const Creative&);

  Creative(Creative&&) noexcept;
  Creative& operator=(Creative&&) noexcept;

  ~Creative();

  WallpaperType wallpaper_type;
  GURL url;
  base::FilePath file_path;
  gfx::Point focal_point;
  brave_ads::ConditionMatcherMap condition_matchers;
  std::string background_color;

  std::string creative_instance_id;

  Logo logo;
  std::optional<gfx::Rect> viewbox;
};

struct Campaign {
  Campaign();

  Campaign(const Campaign&);
  Campaign& operator=(const Campaign&);

  Campaign(Campaign&&) noexcept;
  Campaign& operator=(Campaign&&) noexcept;

  ~Campaign();

  [[nodiscard]] bool IsValid() const;

  std::string campaign_id;
  std::vector<Creative> creatives;
};

// For SI, campaign list can have multiple items.
// For SR, campaign list has only one item.
struct NTPSponsoredImagesData {
  NTPSponsoredImagesData();
  NTPSponsoredImagesData(const base::Value::Dict& dict,
                         const base::FilePath& installed_dir);

  NTPSponsoredImagesData(const NTPSponsoredImagesData&);
  NTPSponsoredImagesData& operator=(const NTPSponsoredImagesData&);

  NTPSponsoredImagesData(NTPSponsoredImagesData&&) noexcept;
  NTPSponsoredImagesData& operator=(NTPSponsoredImagesData&&) noexcept;

  ~NTPSponsoredImagesData();

  [[nodiscard]] bool IsValid() const;

  void ParseCampaigns(const base::Value::List& list,
                      const base::FilePath& installed_dir);
  std::optional<Campaign> ParseCampaign(const base::Value::Dict& dict,
                                        const base::FilePath& installed_dir);

  void ParseSuperReferrals(const base::Value::Dict& dict,
                           const base::FilePath& installed_dir);

  std::optional<base::Value::Dict> GetBackgroundAt(size_t campaign_index,
                                                   size_t creative_index) const;
  std::optional<base::Value::Dict> GetBackground(
      const brave_ads::NewTabPageAdInfo& ad_info);

  bool IsSuperReferral() const;

  bool AdInfoMatchesSponsoredImage(const brave_ads::NewTabPageAdInfo& ad_info,
                                   size_t campaign_index,
                                   size_t creative_index) const;

  std::string url_prefix;

  std::vector<Campaign> campaigns;

  // SR only properties.
  std::string theme_name;
  std::vector<TopSite> top_sites;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_

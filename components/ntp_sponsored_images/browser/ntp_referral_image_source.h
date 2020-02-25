/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGE_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGE_SOURCE_H_

#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/public/browser/url_data_source.h"

namespace ntp_sponsored_images {

class NTPReferralImagesService;

// This serves branded image data.
class NTPReferralImageSource : public content::URLDataSource {
 public:
  explicit NTPReferralImageSource(NTPReferralImagesService* service);

  ~NTPReferralImageSource() override;

  NTPReferralImageSource(const NTPReferralImageSource&) = delete;
  NTPReferralImageSource& operator=(const NTPReferralImageSource&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(NTPReferralImagesServiceTest, ImageSourceTest);

  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) override;
  bool AllowCaching() override;

  void OnGotImageFile(GotDataCallback callback,
                      base::Optional<std::string> input);
  bool IsValidPath(const std::string& path) const;
  bool IsLogoPath(const std::string& path) const;
  bool IsWallpaperPath(const std::string& path) const;
  int GetIconFileIndexFromPath(const std::string& path) const;
  bool IsIconPath(const std::string& path) const;
  int GetWallpaperIndexFromPath(const std::string& path) const;

  NTPReferralImagesService* service_;  // not owned
  base::WeakPtrFactory<NTPReferralImageSource> weak_factory_;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGE_SOURCE_H_

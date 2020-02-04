/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGE_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGE_SOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/public/browser/url_data_source.h"

namespace ntp_sponsored_images {

class NTPSponsoredImagesService;

// This serves branded image data.
class NTPSponsoredImageSource : public content::URLDataSource {
 public:
  explicit NTPSponsoredImageSource(NTPSponsoredImagesService* service);

  ~NTPSponsoredImageSource() override;

  NTPSponsoredImageSource(const NTPSponsoredImageSource&) = delete;
  NTPSponsoredImageSource& operator=(
      const NTPSponsoredImageSource&) = delete;

 private:
  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) override;

  void OnGotImageFile(GotDataCallback callback,
                      base::Optional<std::string> input);
  bool IsValidPath(const std::string& path) const;
  bool IsLogoPath(const std::string& path) const;
  bool IsWallpaperPath(const std::string& path) const;
  int GetWallpaperIndexFromPath(const std::string& path) const;

  NTPSponsoredImagesService* service_;  // not owned
  base::WeakPtrFactory<NTPSponsoredImageSource> weak_factory_;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGE_SOURCE_H_

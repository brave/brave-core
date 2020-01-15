/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGE_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGE_SOURCE_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_internal_data.h"
#include "content/public/browser/url_data_source.h"

class NTPSponsoredImagesComponentManager;

// This serves branded image data.
// This referes with weak ptr because both can have different life cycle.
class NTPSponsoredImageSource : public content::URLDataSource {
 public:
  explicit NTPSponsoredImageSource(
      const NTPSponsoredImagesInternalData& internal_images_data);

  ~NTPSponsoredImageSource() override;

  NTPSponsoredImageSource(const NTPSponsoredImageSource&) = delete;
  NTPSponsoredImageSource& operator=(
      const NTPSponsoredImageSource&) = delete;

 private:
  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const std::string& path,
                        const content::WebContents::Getter& wc_getter,
                        const GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) override;

  void OnGotImageFile(const GotDataCallback& callback,
                      base::Optional<std::string> input);
  bool IsValidPath(const std::string& path) const;
  bool IsLogoPath(const std::string& path) const;
  bool IsWallpaperPath(const std::string& path) const;
  int GetWallpaperIndexFromPath(const std::string& path) const;

  const NTPSponsoredImagesInternalData images_data_;
  base::WeakPtrFactory<NTPSponsoredImageSource> weak_factory_;
};

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGE_SOURCE_H_

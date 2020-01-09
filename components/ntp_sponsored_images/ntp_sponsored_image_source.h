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
#include "content/public/browser/url_data_source.h"

class NTPSponsoredImagesComponentManager;

// This serves branded image data.
// This referes with weak ptr because both can have different life cycle.
class NTPSponsoredImageSource : public content::URLDataSource {
 public:
  enum class Type {
    TYPE_LOGO,
    TYPE_WALLPAPER,
  };

  explicit NTPSponsoredImageSource(
      base::WeakPtr<NTPSponsoredImagesComponentManager> manager,
      const base::FilePath& image_file_path,
      size_t wallpaper_index,
      Type type);
  ~NTPSponsoredImageSource() override;

  NTPSponsoredImageSource(const NTPSponsoredImageSource&) = delete;
  NTPSponsoredImageSource operator=(
      const NTPSponsoredImageSource&) = delete;

 private:
  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const std::string& path,
                        const content::WebContents::Getter& wc_getter,
                        const GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) override;

  bool IsLogoType() const;
  std::string GetWallpaperPath() const;
  void OnGotImageFile(const GotDataCallback& callback,
                      base::Optional<std::string> input);

  base::WeakPtr<NTPSponsoredImagesComponentManager> manager_;
  const base::FilePath image_file_path_;
  size_t wallpaper_index_;  // Only used for wallpaper type.
  Type type_;

  base::WeakPtrFactory<NTPSponsoredImageSource> weak_factory_;
};

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGE_SOURCE_H_

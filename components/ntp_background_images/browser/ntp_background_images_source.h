/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SOURCE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/public/browser/url_data_source.h"

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

class NTPBackgroundImagesService;

// This serves background image data.
class NTPBackgroundImagesSource : public content::URLDataSource {
 public:
  explicit NTPBackgroundImagesSource(NTPBackgroundImagesService* service);

  ~NTPBackgroundImagesSource() override;

  NTPBackgroundImagesSource(const NTPBackgroundImagesSource&) = delete;
  NTPBackgroundImagesSource& operator=(
      const NTPBackgroundImagesSource&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, BasicTest);

  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) override;
  bool AllowCaching() override;

  void GetImageFile(const base::FilePath& image_file_path,
                    GotDataCallback callback);
  void OnGotImageFile(GotDataCallback callback,
                      base::Optional<std::string> input);
  bool IsValidPath(const std::string& path) const;
  bool IsLogoPath(const std::string& path) const;
  bool IsWallpaperPath(const std::string& path) const;
  int GetWallpaperIndexFromPath(const std::string& path) const;
  bool IsTopSiteFaviconPath(const std::string& path) const;
  base::FilePath GetTopSiteFaviconFilePath(const std::string& path) const;

  NTPBackgroundImagesService* service_;  // not owned
  base::WeakPtrFactory<NTPBackgroundImagesSource> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SOURCE_H_

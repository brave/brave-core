/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_RICH_MEDIA_SOURCE_IOS_H_
#define BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_RICH_MEDIA_SOURCE_IOS_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/ios/web/webui/brave_url_data_source_ios.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

class NTPBackgroundImagesService;

// This class is responsible for providing sponsored rich media content from the
// file system to the new tab page.

class NTPSponsoredRichMediaSourceIOS final : public BraveURLDataSourceIOS {
 public:
  // TODO(aseren): Add this function to WebUIIOSDataSource for rich media
  // background.
  // static NTPSponsoredRichMediaSourceIOS* CreateAndAdd(
  //     web::BrowserState* browser_state,
  //     NTPBackgroundImagesService* background_images_service);

  explicit NTPSponsoredRichMediaSourceIOS(
      NTPBackgroundImagesService* background_images_service);

  NTPSponsoredRichMediaSourceIOS(const NTPSponsoredRichMediaSourceIOS&) =
      delete;
  NTPSponsoredRichMediaSourceIOS& operator=(
      const NTPSponsoredRichMediaSourceIOS&) = delete;

  ~NTPSponsoredRichMediaSourceIOS() override;

  // BraveURLDataSourceIOS:
  std::string GetSource() const override;
  void StartDataRequest(const std::string& path,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool AllowCaching() const override;
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const override;

 private:
  void ReadFileCallback(GotDataCallback callback,
                        std::optional<std::string> input);

  void AllowAccess(const base::FilePath& file_path, GotDataCallback callback);
  void DenyAccess(GotDataCallback callback);

  const raw_ptr<NTPBackgroundImagesService>
      background_images_service_;  // Not owned.

  base::WeakPtrFactory<NTPSponsoredRichMediaSourceIOS> weak_factory_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_RICH_MEDIA_SOURCE_IOS_H_

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_H_

#include <memory>
#include <string>

#include "components/keyed_service/core/keyed_service.h"

namespace base {
class Value;
class FilePath;
}  // namespace base

class GURL;

namespace ntp_background_images {

// TODO(sko) Rename this to NTPCustomBackgroundService. It's dealing with
// not only images but also colors.
// https://github.com/brave/brave-browser/issues/24926
class NTPCustomBackgroundImagesService : public KeyedService {
 public:
  class Delegate {
   public:
    virtual bool IsCustomImageBackgroundEnabled() = 0;
    virtual base::FilePath GetCustomBackgroundImageLocalFilePath() = 0;

    virtual bool IsColorBackgroundEnabled() = 0;
    virtual std::string GetColor() = 0;

    virtual ~Delegate() = default;
  };

  explicit NTPCustomBackgroundImagesService(std::unique_ptr<Delegate> delegate);
  ~NTPCustomBackgroundImagesService() override;

  NTPCustomBackgroundImagesService(const NTPCustomBackgroundImagesService&) =
      delete;
  NTPCustomBackgroundImagesService& operator=(
      const NTPCustomBackgroundImagesService&) = delete;

  bool ShouldShowCustomBackground() const;
  base::Value GetBackground() const;
  base::FilePath GetImageFilePath();

 private:
  // KeyedService overrides:
  void Shutdown() override;

  std::unique_ptr<Delegate> delegate_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_H_

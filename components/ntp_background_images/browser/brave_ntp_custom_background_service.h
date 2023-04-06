// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}  // namespace base

class GURL;

namespace ntp_background_images {

class BraveNTPCustomBackgroundService : public KeyedService {
 public:
  class Delegate {
   public:
    virtual bool IsCustomImageBackgroundEnabled() const = 0;
    virtual base::FilePath GetCustomBackgroundImageLocalFilePath(
        const GURL& url) const = 0;
    virtual GURL GetCustomBackgroundImageURL() const = 0;

    virtual bool IsColorBackgroundEnabled() const = 0;
    virtual std::string GetColor() const = 0;
    virtual bool ShouldUseRandomValue() const = 0;

    virtual bool HasPreferredBraveBackground() const = 0;
    virtual base::Value::Dict GetPreferredBraveBackground() const = 0;

    virtual ~Delegate() = default;
  };

  explicit BraveNTPCustomBackgroundService(std::unique_ptr<Delegate> delegate);
  ~BraveNTPCustomBackgroundService() override;

  BraveNTPCustomBackgroundService(const BraveNTPCustomBackgroundService&) =
      delete;
  BraveNTPCustomBackgroundService& operator=(
      const BraveNTPCustomBackgroundService&) = delete;

  bool ShouldShowCustomBackground() const;
  base::Value::Dict GetBackground() const;
  base::FilePath GetImageFilePath(const GURL& url);

 private:
  // KeyedService overrides:
  void Shutdown() override;

  std::unique_ptr<Delegate> delegate_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_H_

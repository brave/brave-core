// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_H_

#include <memory>

#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}  // namespace base

class GURL;

namespace ntp_background_images {

// Defined in ntp_custom_background_delegate.h.
class NTPCustomBackgroundDelegate;

class BraveNTPCustomBackgroundService : public KeyedService {
 public:
  explicit BraveNTPCustomBackgroundService(
      std::unique_ptr<NTPCustomBackgroundDelegate> delegate);
  ~BraveNTPCustomBackgroundService() override;

  BraveNTPCustomBackgroundService(const BraveNTPCustomBackgroundService&) =
      delete;
  BraveNTPCustomBackgroundService& operator=(
      const BraveNTPCustomBackgroundService&) = delete;

  bool ShouldShowCustomBackground() const;
  base::DictValue GetBackground() const;
  base::FilePath GetImageFilePath(const GURL& url);

 private:
  // KeyedService overrides:
  void Shutdown() override;

  std::unique_ptr<NTPCustomBackgroundDelegate> delegate_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_H_

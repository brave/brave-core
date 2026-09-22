/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_CUSTOM_BACKGROUND_DELEGATE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_CUSTOM_BACKGROUND_DELEGATE_H_

#include <string>

#include "base/values.h"

namespace base {
class FilePath;
}  // namespace base

class GURL;

namespace ntp_background_images {

// Lets platform-specific code (profile prefs, file storage) answer these
// questions without BraveNTPCustomBackgroundService depending on that code
// directly.
class NTPCustomBackgroundDelegate {
 public:
  virtual bool IsCustomImageBackgroundEnabled() const = 0;
  virtual base::FilePath GetCustomBackgroundImageLocalFilePath(
      const GURL& url) const = 0;
  virtual GURL GetCustomBackgroundImageURL() const = 0;

  virtual bool IsColorBackgroundEnabled() const = 0;
  virtual std::string GetColor() const = 0;
  virtual bool ShouldUseRandomValue() const = 0;

  virtual bool HasPreferredBraveBackground() const = 0;
  virtual base::DictValue GetPreferredBraveBackground() const = 0;

  virtual ~NTPCustomBackgroundDelegate() = default;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_CUSTOM_BACKGROUND_DELEGATE_H_

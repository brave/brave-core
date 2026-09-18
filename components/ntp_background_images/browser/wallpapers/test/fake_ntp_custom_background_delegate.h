/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_TEST_FAKE_NTP_CUSTOM_BACKGROUND_DELEGATE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_TEST_FAKE_NTP_CUSTOM_BACKGROUND_DELEGATE_H_

#include <string>

#include "base/values.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_custom_background_delegate.h"
#include "url/gurl.h"

namespace ntp_background_images::test {

class FakeNTPCustomBackgroundDelegate final
    : public NTPCustomBackgroundDelegate {
 public:
  FakeNTPCustomBackgroundDelegate();

  FakeNTPCustomBackgroundDelegate(
      const FakeNTPCustomBackgroundDelegate&) = delete;
  FakeNTPCustomBackgroundDelegate& operator=(
      const FakeNTPCustomBackgroundDelegate&) = delete;

  ~FakeNTPCustomBackgroundDelegate() override;

  // NTPCustomBackgroundDelegate:
  bool IsCustomImageBackgroundEnabled() const override;
  base::FilePath GetCustomBackgroundImageLocalFilePath(
      const GURL& url) const override;
  GURL GetCustomBackgroundImageURL() const override;
  bool IsColorBackgroundEnabled() const override;
  std::string GetColor() const override;
  bool ShouldUseRandomValue() const override;
  bool HasPreferredBraveBackground() const override;
  base::DictValue GetPreferredBraveBackground() const override;

  void set_is_custom_image_background_enabled(bool enabled) {
    is_custom_image_background_enabled_ = enabled;
  }
  void set_is_color_background_enabled(bool enabled) {
    is_color_background_enabled_ = enabled;
  }
  void set_color(const std::string& color) { color_ = color; }
  void set_should_use_random_value(bool value) { should_use_random_ = value; }
  void SetPreferredBraveBackground(bool has_preferred, base::DictValue value);

 private:
  bool is_custom_image_background_enabled_ = false;
  bool is_color_background_enabled_ = false;
  std::string color_;
  bool should_use_random_ = false;
  bool has_preferred_brave_background_ = false;
  base::DictValue preferred_brave_background_;
};

}  // namespace ntp_background_images::test

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_TEST_FAKE_NTP_CUSTOM_BACKGROUND_DELEGATE_H_

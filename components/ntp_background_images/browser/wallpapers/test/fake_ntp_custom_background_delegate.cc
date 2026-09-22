/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/test/fake_ntp_custom_background_delegate.h"

#include <utility>

#include "base/files/file_path.h"

namespace ntp_background_images::test {

FakeNTPCustomBackgroundDelegate::FakeNTPCustomBackgroundDelegate() = default;

FakeNTPCustomBackgroundDelegate::~FakeNTPCustomBackgroundDelegate() = default;

bool FakeNTPCustomBackgroundDelegate::IsCustomImageBackgroundEnabled() const {
  return is_custom_image_background_enabled_;
}

base::FilePath
FakeNTPCustomBackgroundDelegate::GetCustomBackgroundImageLocalFilePath(
    const GURL& /*url*/) const {
  return {};
}

GURL FakeNTPCustomBackgroundDelegate::GetCustomBackgroundImageURL() const {
  return GURL("chrome://custom-wallpaper/foo.jpg");
}

bool FakeNTPCustomBackgroundDelegate::IsColorBackgroundEnabled() const {
  return is_color_background_enabled_;
}

std::string FakeNTPCustomBackgroundDelegate::GetColor() const {
  return color_;
}

bool FakeNTPCustomBackgroundDelegate::ShouldUseRandomValue() const {
  return should_use_random_;
}

bool FakeNTPCustomBackgroundDelegate::HasPreferredBraveBackground() const {
  return has_preferred_brave_background_;
}

base::DictValue
FakeNTPCustomBackgroundDelegate::GetPreferredBraveBackground() const {
  return preferred_brave_background_.Clone();
}

void FakeNTPCustomBackgroundDelegate::SetPreferredBraveBackground(
    bool has_preferred,
    base::DictValue value) {
  has_preferred_brave_background_ = has_preferred;
  preferred_brave_background_ = std::move(value);
}

}  // namespace ntp_background_images::test

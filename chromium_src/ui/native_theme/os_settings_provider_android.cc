/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <ui/native_theme/os_settings_provider_android.cc>

namespace ui {

std::optional<SkColor> OsSettingsProviderAndroid::AccentColor() const {
  return accent_color_;
}

void OsSettingsProviderAndroid::SetAccentColor(
    std::optional<SkColor> accent_color) {
  if (accent_color_ == accent_color) {
    return;
  }
  accent_color_ = accent_color;
  NotifyOnSettingsChanged();
}

}  // namespace ui

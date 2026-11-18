/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include <chrome/browser/ui/extensions/icon_with_badge_image_source.cc>

// Implement default virtual methods
std::optional<int> IconWithBadgeImageSource::GetCustomGraphicSize() {
  return std::nullopt;
}

std::optional<int> IconWithBadgeImageSource::GetCustomGraphicXOffset() {
  return std::nullopt;
}

std::optional<int> IconWithBadgeImageSource::GetCustomGraphicYOffset() {
  return std::nullopt;
}

void IconWithBadgeImageSource::ApplyCustomGraphicOffsets(int& x_offset,
                                                         int& y_offset) {
  if (const auto custom_x_offset = GetCustomGraphicXOffset()) {
    x_offset = *custom_x_offset;
  }
  if (const auto custom_y_offset = GetCustomGraphicYOffset()) {
    y_offset = *custom_y_offset;
  }
}

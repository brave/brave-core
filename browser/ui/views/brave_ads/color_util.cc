/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/color_util.h"

#include <array>

#include "base/compiler_specific.h"
#include "base/containers/span_reader.h"
#include "base/containers/span_writer.h"
#include "base/strings/string_number_conversions.h"

namespace brave_ads {

bool RgbStringToSkColor(std::string_view rgb, SkColor* color) {
  CHECK(color);

  // Expect three RGB color components with length == 2, e.g. 42fe4c.
  constexpr size_t kColorComponentsCount = 3;
  constexpr size_t kColorComponentLen = 2;

  if (rgb.size() != kColorComponentsCount * kColorComponentLen) {
    return false;
  }

  std::array<uint32_t, kColorComponentsCount> hex;
  base::SpanReader<const char> reader(rgb);
  base::SpanWriter<uint32_t> writer(hex);
  while (auto component = reader.Read<kColorComponentLen>()) {
    uint32_t value;
    if (!base::HexStringToUInt(base::as_string_view(*component), &value)) {
      return false;
    }
    writer.Write(value);
  }
  *color = SkColorSetRGB(hex[0], hex[1], hex[2]);
  return true;
}

}  // namespace brave_ads

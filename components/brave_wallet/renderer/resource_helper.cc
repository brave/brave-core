/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/resource_helper.h"

#include <optional>
#include <vector>

#include "base/base64.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/codec/png_codec.h"

namespace brave_wallet {

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return std::string(resource_bundle.GetRawDataResource(id));
}

std::optional<std::string> LoadImageResourceAsDataUrl(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  auto image = resource_bundle.GetImageNamed(id);
  if (image.IsEmpty()) {
    return std::nullopt;
  }

  std::optional<std::vector<uint8_t>> data =
      gfx::PNGCodec::EncodeBGRASkBitmap(image.AsBitmap(),
                                        /*discard_transparency=*/false);
  if (!data) {
    return std::nullopt;
  }

  return "data:image/png;base64," + base::Base64Encode(*data);
}

}  // namespace brave_wallet

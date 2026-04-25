/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/qr_code_generator/qrcode_models.h"

#include "components/qr_code_generator/bitmap_generator.h"

namespace qr_code_generator {

GenerateQRCodeOptions::GenerateQRCodeOptions()
    : data(),
      should_render(true),
      render_dino(true),
      render_module_style(ModuleStyle::kSquares),
      render_locator_style(LocatorStyle::kSquare) {}

GenerateQRCodeOptions::GenerateQRCodeOptions(const std::string& data,
                                             bool should_render,
                                             bool render_dino,
                                             ModuleStyle render_module_style,
                                             LocatorStyle render_locator_style)
    : data(data),
      should_render(should_render),
      render_dino(render_dino),
      render_module_style(render_module_style),
      render_locator_style(render_locator_style) {}

GenerateQRCodeOptions::~GenerateQRCodeOptions() = default;

GenerateQRCodeResult::GenerateQRCodeResult()
    : error_code(Error::kUnknownError), bitmap(), data(), data_size() {}

GenerateQRCodeResult::GenerateQRCodeResult(
    Error error_code,
    const SkBitmap& bitmap,
    const std::vector<std::uint8_t>& data,
    const gfx::Size& data_size)
    : error_code(error_code),
      bitmap(bitmap),
      data(data),
      data_size(data_size) {}

GenerateQRCodeResult::~GenerateQRCodeResult() = default;

}  // namespace qr_code_generator

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/qr_code_generator/qrcode_models.h"

namespace qrcode_generator {

GenerateQRCodeRequest::GenerateQRCodeRequest()
    : data(),
      should_render(true),
      render_dino(true),
      render_module_style(ModuleStyle::DEFAULT_SQUARES),
      render_locator_style(LocatorStyle::DEFAULT_SQUARE) {}

GenerateQRCodeRequest::GenerateQRCodeRequest(const std::string& data,
                                             bool should_render,
                                             bool render_dino,
                                             ModuleStyle render_module_style,
                                             LocatorStyle render_locator_style)
    : data(data),
      should_render(should_render),
      render_dino(render_dino),
      render_module_style(render_module_style),
      render_locator_style(render_locator_style) {}

GenerateQRCodeRequest::~GenerateQRCodeRequest() = default;

GenerateQRCodeResponse::GenerateQRCodeResponse()
    : error_code(QRCodeGeneratorError::NONE), bitmap(), data(), data_size() {}

GenerateQRCodeResponse::GenerateQRCodeResponse(
    QRCodeGeneratorError error_code,
    const SkBitmap& bitmap,
    const std::vector<std::uint8_t>& data,
    const gfx::Size& data_size)
    : error_code(error_code),
      bitmap(bitmap),
      data(data),
      data_size(data_size) {}

GenerateQRCodeResponse::~GenerateQRCodeResponse() = default;

}  // namespace qrcode_generator

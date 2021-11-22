/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_validator.h"

#include <string>

namespace brave_sync {

QrCodeDataValidationResult QrCodeDataValidator::ValidateQrData(
    const std::string& qr_data_string) {
  return QrCodeDataValidationResult::kInvalidUnknowReason;
}

}  // namespace brave_sync

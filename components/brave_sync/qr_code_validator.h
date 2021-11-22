/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_QR_CODE_VALIDATOR_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_QR_CODE_VALIDATOR_H_

#include <string>

namespace brave_sync {

enum class QrCodeDataValidationResult {
  kValid = 0,
  kInvalidUnknowReason = 1,
  kNotWellFormed = 2,
  kVersionNotRecognized = 3,
  kVersionDeprecated = 4,
  kExpired = 5,
  kValidForTooLong = 6,
};

class QrCodeDataValidator {
 public:
  static QrCodeDataValidationResult ValidateQrData(
      const std::string& qr_data_string);
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_QR_CODE_VALIDATOR_H_

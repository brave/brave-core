/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_QR_CODE_DATA_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_QR_CODE_DATA_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "base/values.h"

namespace brave_sync {

class QrCodeData {
 public:
  static constexpr int kCurrentQrCodeDataVersion = 2;
  static constexpr int kMinutesFromNowForValidCode = 30;

  int version;
  std::string sync_code_hex;
  base::Time not_after;

  static std::unique_ptr<QrCodeData> CreateWithActualDate(
      const std::string& sync_code_hex);

  std::string ToJson();
  static std::unique_ptr<QrCodeData> FromJson(const std::string& json_string);

  static base::Time FromEpochSeconds(int64_t seconds_since_epoch);
  static int64_t ToEpochSeconds(const base::Time& time);

 private:
  QrCodeData();
  QrCodeData(const std::string& sync_code_hex, const base::Time& not_after);

  base::Value::Dict ToValue() const;
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_QR_CODE_DATA_H_

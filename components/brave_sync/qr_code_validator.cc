/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_validator.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/qr_code_data.h"

namespace brave_sync {

namespace {

static constexpr base::TimeDelta kIntervalForValidForTooLong =
    base::Minutes(60);
static constexpr size_t kSeedBytesCount = 32u;
static constexpr size_t kPassphraseWordsCount = 24u;
static constexpr char kQRv1SunsetDate[] = "Sat, 1 Jan 2022 00:00:00 GMT";

}  // namespace

base::Time QrCodeDataValidator::qr_v1_sunset_day_;

base::Time QrCodeDataValidator::GetQRv1SunsetDay() {
  if (qr_v1_sunset_day_.is_null()) {
    bool convert_result =
        base::Time::FromUTCString(kQRv1SunsetDate, &qr_v1_sunset_day_);
    CHECK(convert_result);
  }

  CHECK(!qr_v1_sunset_day_.is_null());

  return qr_v1_sunset_day_;
}

QrCodeDataValidationResult QrCodeDataValidator::ValidateQrDataJson(
    const std::string& qr_data_string) {
  auto now = base::Time::Now();
  auto qr_code_data = QrCodeData::FromJson(qr_data_string);

  if (!qr_code_data) {
    if (IsValidSeedHex(qr_data_string)) {
      if (now < GetQRv1SunsetDay()) {
        return QrCodeDataValidationResult::kValid;
      } else {
        return QrCodeDataValidationResult::kVersionDeprecated;
      }
    }
    return QrCodeDataValidationResult::kNotWellFormed;
  }

  if (qr_code_data->sync_code_hex.empty()) {
    return QrCodeDataValidationResult::kNotWellFormed;
  }

  if (qr_code_data->version < QrCodeData::kCurrentQrCodeDataVersion) {
    return QrCodeDataValidationResult::kVersionDeprecated;
  } else if (qr_code_data->version > QrCodeData::kCurrentQrCodeDataVersion) {
    return QrCodeDataValidationResult::kVersionNotRecognized;
  }

  if (!IsValidSeedHex(qr_code_data->sync_code_hex)) {
    return QrCodeDataValidationResult::kNotWellFormed;
  }

  if (now > qr_code_data->not_after) {
    return QrCodeDataValidationResult::kExpired;
  }

  if (qr_code_data->not_after - now > kIntervalForValidForTooLong) {
    return QrCodeDataValidationResult::kValidForTooLong;
  }

  return QrCodeDataValidationResult::kValid;
}

bool QrCodeDataValidator::IsValidSeedHex(const std::string& seed_hex) {
  std::string seed_hex_trimmed;
  base::TrimString(seed_hex, " \n\t", &seed_hex_trimmed);

  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(seed_hex_trimmed, &bytes)) {
    return false;
  }

  if (bytes.size() != kSeedBytesCount) {
    return false;
  }

  std::string sync_code_words =
      brave_sync::crypto::PassphraseFromBytes32(bytes);
  if (sync_code_words.empty()) {
    return false;
  }

  const std::vector<std::string_view> words = base::SplitStringPiece(
      sync_code_words, " ", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  if (words.size() != kPassphraseWordsCount) {
    NOTREACHED_IN_MIGRATION() << "Passphrase words number is " << words.size();
    return false;
  }

  return true;
}

}  // namespace brave_sync

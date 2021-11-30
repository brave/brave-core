/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_data.h"

#include <memory>
#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"

// Example of the JSON:
// {
//   "version": "2",
//   "sync_code_hex" : "<current hex code>",
//   "not_after": "1637080050"
// }

namespace brave_sync {

QrCodeData::QrCodeData() : version(kCurrentQrCodeDataVersion) {}

QrCodeData::QrCodeData(const std::string& sync_code_hex,
                       const base::Time& not_after)
    : version(kCurrentQrCodeDataVersion),
      sync_code_hex(sync_code_hex),
      not_after(not_after) {}

base::Time QrCodeData::FromEpochSeconds(int64_t seconds_since_epoch) {
  return base::Time::FromJavaTime(seconds_since_epoch * 1000);
}

int64_t QrCodeData::ToEpochSeconds(const base::Time& time) {
  return time.ToJavaTime() / 1000;
}

std::unique_ptr<QrCodeData> QrCodeData::CreateWithActualDate(
    const std::string& sync_code_hex) {
  return std::unique_ptr<QrCodeData>(new QrCodeData(
      sync_code_hex,
      base::Time::Now() + base::Minutes(kMinutesFromNowForValidCode)));
}

std::unique_ptr<base::DictionaryValue> QrCodeData::ToValue() const {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetString("version", base::NumberToString(version));
  dict->SetString("sync_code_hex", sync_code_hex);
  dict->SetString("not_after", base::NumberToString(ToEpochSeconds(not_after)));
  return dict;
}

std::string QrCodeData::ToJson() {
  auto dict = ToValue();
  CHECK(dict);

  std::string json_string;
  if (!base::JSONWriter::Write(*dict.get(), &json_string)) {
    VLOG(1) << "Writing QR data to JSON failed";
    json_string = std::string();
  }

  return json_string;
}

std::unique_ptr<QrCodeData> QrCodeData::FromJson(
    const std::string& json_string) {
  auto qr_data = std::unique_ptr<QrCodeData>(new QrCodeData());

  absl::optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value) {
    VLOG(1) << "Could not parse string " << json_string;
    return nullptr;
  }

  if (!value->is_dict()) {
    VLOG(1) << "Invalid JSON: " << *value;
    return nullptr;
  }

  const std::string* version_value = value->FindStringKey("version");
  if (!version_value) {
    VLOG(1) << "Missing version";
    return nullptr;
  }

  int version;
  if (!base::StringToInt(*version_value, &version)) {
    VLOG(1) << "Version has wrong format";
    return nullptr;
  }
  qr_data->version = version;

  const std::string* sync_code_hex_value =
      value->FindStringKey("sync_code_hex");
  if (!sync_code_hex_value) {
    VLOG(1) << "Missing sync code hex";
    return nullptr;
  }
  qr_data->sync_code_hex = *sync_code_hex_value;

  const std::string* not_after_string = value->FindStringKey("not_after");
  if (!not_after_string) {
    VLOG(1) << "Missing not after time";
    return nullptr;
  }

  int64_t not_after_int;
  if (!base::StringToInt64(*not_after_string, &not_after_int)) {
    VLOG(1) << "Wrong format for not after time";
    return nullptr;
  }

  qr_data->not_after = FromEpochSeconds(not_after_int);

  return qr_data;
}

}  // namespace brave_sync

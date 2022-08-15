/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/conversions/legacy_conversions_migration.h"

#include <string>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/conversions/conversion_queue_database_table.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace conversions {

namespace {

constexpr char kFilename[] = "ad_conversions.json";

constexpr char kListKey[] = "ad_conversions";
constexpr char kTimestampKey[] = "timestamp";
constexpr char kCreativeSetIdKey[] = "creative_set_id";
constexpr char kCreativeInstanceIdKey[] = "uuid";

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedConversionState);
}

void FailedToMigrate(InitializeCallback callback) {
  callback(/* success */ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedConversionState, true);
  callback(/* success */ true);
}

absl::optional<ConversionQueueItemInfo> GetFromDictionary(
    const base::Value::Dict& dict) {
  // Timestamp
  const std::string* timestamp_value = dict.FindString(kTimestampKey);
  if (!timestamp_value) {
    return absl::nullopt;
  }

  double timestamp;
  if (!base::StringToDouble(*timestamp_value, &timestamp)) {
    return absl::nullopt;
  }

  // Creative set id
  const std::string* creative_set_id_value = dict.FindString(kCreativeSetIdKey);
  if (!creative_set_id_value) {
    return absl::nullopt;
  }

  // Creative instance id
  const auto* creative_instance_id_value =
      dict.FindString(kCreativeInstanceIdKey);
  if (!creative_instance_id_value) {
    return absl::nullopt;
  }

  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.creative_set_id = *creative_set_id_value;
  conversion_queue_item.creative_instance_id = *creative_instance_id_value;
  conversion_queue_item.process_at = base::Time::FromDoubleT(timestamp);

  return conversion_queue_item;
}

absl::optional<ConversionQueueItemList> GetFromList(
    const base::Value::List& list) {
  ConversionQueueItemList conversion_queue_items;

  for (const auto& item : list) {
    const base::Value::Dict* dict = item.GetIfDict();
    if (!dict) {
      return absl::nullopt;
    }

    const absl::optional<ConversionQueueItemInfo> conversion_queue_item =
        GetFromDictionary(*dict);
    if (!conversion_queue_item) {
      return absl::nullopt;
    }

    conversion_queue_items.push_back(*conversion_queue_item);
  }

  return conversion_queue_items;
}

absl::optional<ConversionQueueItemList> FromJson(const std::string& json) {
  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return absl::nullopt;
  }
  const base::Value::Dict& dict = root->GetDict();

  const base::Value::List* list = dict.FindList(kListKey);
  if (!list) {
    return absl::nullopt;
  }

  return GetFromList(*list);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    callback(/* success */ true);
    return;
  }

  BLOG(3, "Loading conversion state");

  AdsClientHelper::GetInstance()->Load(
      kFilename, [=](const bool success, const std::string& json) {
        if (!success) {
          // Conversion state does not exist
          SuccessfullyMigrated(callback);
          return;
        }

        const absl::optional<ConversionQueueItemList> conversion_queue_items =
            FromJson(json);
        if (!conversion_queue_items) {
          BLOG(0, "Failed to parse conversion state");
          FailedToMigrate(callback);
          return;
        }

        BLOG(3, "Successfully loaded conversion state");

        BLOG(1, "Migrating conversion state");

        database::table::ConversionQueue conversion_queue;
        conversion_queue.Save(*conversion_queue_items, [=](const bool success) {
          if (!success) {
            BLOG(0, "Failed to save conversion state");
            FailedToMigrate(callback);
            return;
          }

          BLOG(3, "Successfully migrated conversion state");
          SuccessfullyMigrated(callback);
        });
      });
}

}  // namespace conversions
}  // namespace ads

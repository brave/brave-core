/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/conversions/legacy_conversions_migration.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::conversions {

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
  std::move(callback).Run(/*success*/ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedConversionState, true);
  std::move(callback).Run(/*success*/ true);
}

absl::optional<ConversionQueueItemInfo> GetFromDictionary(
    const base::Value::Dict& dict) {
  // Timestamp
  const std::string* const timestamp = dict.FindString(kTimestampKey);
  if (!timestamp) {
    return absl::nullopt;
  }

  double timestamp_as_double;
  if (!base::StringToDouble(*timestamp, &timestamp_as_double)) {
    return absl::nullopt;
  }

  // Creative set id
  const std::string* const creative_set_id = dict.FindString(kCreativeSetIdKey);
  if (!creative_set_id) {
    return absl::nullopt;
  }

  // Creative instance id
  const auto* const creative_instance_id =
      dict.FindString(kCreativeInstanceIdKey);
  if (!creative_instance_id) {
    return absl::nullopt;
  }

  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.creative_set_id = *creative_set_id;
  conversion_queue_item.creative_instance_id = *creative_instance_id;
  conversion_queue_item.process_at =
      base::Time::FromDoubleT(timestamp_as_double);

  return conversion_queue_item;
}

absl::optional<ConversionQueueItemList> GetFromList(
    const base::Value::List& list) {
  ConversionQueueItemList conversion_queue_items;

  for (const auto& item : list) {
    const auto* const item_dict = item.GetIfDict();
    if (!item_dict) {
      return absl::nullopt;
    }

    const absl::optional<ConversionQueueItemInfo> conversion_queue_item =
        GetFromDictionary(*item_dict);
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

  const auto* const list = root->GetDict().FindList(kListKey);
  if (!list) {
    return absl::nullopt;
  }

  return GetFromList(*list);
}

void OnMigrate(InitializeCallback callback,
               const absl::optional<std::string>& json) {
  if (!json) {
    // Conversion state does not exist
    return SuccessfullyMigrated(std::move(callback));
  }

  const absl::optional<ConversionQueueItemList> conversion_queue_items =
      FromJson(*json);
  if (!conversion_queue_items) {
    BLOG(0, "Failed to parse conversion state");
    return FailedToMigrate(std::move(callback));
  }

  BLOG(1, "Migrating conversion state");

  database::table::ConversionQueue conversion_queue;
  conversion_queue.Save(
      *conversion_queue_items,
      base::BindOnce(
          [](InitializeCallback callback, const bool success) {
            if (!success) {
              BLOG(0, "Failed to save conversion state");
              return FailedToMigrate(std::move(callback));
            }

            BLOG(3, "Successfully migrated conversion state");
            SuccessfullyMigrated(std::move(callback));
          },
          std::move(callback)));
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    return std::move(callback).Run(/*success*/ true);
  }

  AdsClientHelper::GetInstance()->Load(
      kFilename, base::BindOnce(&OnMigrate, std::move(callback)));
}

}  // namespace brave_ads::conversions

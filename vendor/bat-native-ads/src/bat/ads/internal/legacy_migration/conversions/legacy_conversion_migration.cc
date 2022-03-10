/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/conversions/legacy_conversion_migration.h"

#include <string>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace conversions {

namespace {

const char kFilename[] = "ad_conversions.json";

const char kListKey[] = "ad_conversions";
const char kTimestampKey[] = "timestamp";
const char kCreativeSetIdKey[] = "creative_set_id";
const char kCreativeInstanceIdKey[] = "uuid";

absl::optional<ConversionQueueItemInfo> GetFromDictionary(
    const base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  // Timestamp
  const std::string* timestamp_value = dictionary->FindStringKey(kTimestampKey);
  if (!timestamp_value) {
    return absl::nullopt;
  }

  double timestamp;
  if (!base::StringToDouble(*timestamp_value, &timestamp)) {
    return absl::nullopt;
  }

  // Creative set id
  const std::string* creative_set_id_value =
      dictionary->FindStringKey(kCreativeSetIdKey);
  if (!creative_set_id_value) {
    return absl::nullopt;
  }

  // Creative instance id
  const auto* creative_instance_id_value =
      dictionary->FindStringKey(kCreativeInstanceIdKey);
  if (!creative_instance_id_value) {
    return absl::nullopt;
  }

  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.creative_set_id = *creative_set_id_value;
  conversion_queue_item.creative_instance_id = *creative_instance_id_value;
  conversion_queue_item.process_at = base::Time::FromDoubleT(timestamp);

  return conversion_queue_item;
}

absl::optional<ConversionQueueItemList> GetFromList(const base::Value* list) {
  DCHECK(list);
  DCHECK(list->is_list());

  ConversionQueueItemList conversion_queue_items;

  for (const auto& value : list->GetList()) {
    if (!value.is_dict()) {
      return absl::nullopt;
    }

    const base::DictionaryValue* dictionary = nullptr;
    value.GetAsDictionary(&dictionary);
    if (!dictionary) {
      return absl::nullopt;
    }

    const absl::optional<ConversionQueueItemInfo> conversion_queue_item =
        GetFromDictionary(dictionary);
    if (!conversion_queue_item) {
      return absl::nullopt;
    }

    conversion_queue_items.push_back(conversion_queue_item.value());
  }

  return conversion_queue_items;
}

absl::optional<ConversionQueueItemList> FromJson(const std::string& json) {
  const absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return absl::nullopt;
  }

  const base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return absl::nullopt;
  }

  const base::Value* list = dictionary->FindListKey(kListKey);
  if (!list) {
    return absl::nullopt;
  }

  return GetFromList(list);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (AdsClientHelper::Get()->GetBooleanPref(
          prefs::kHasMigratedConversionState)) {
    callback(/* success */ true);
    return;
  }

  BLOG(3, "Loading conversion state");

  AdsClientHelper::Get()->Load(
      kFilename, [=](const bool success, const std::string& json) {
        if (!success) {
          // Conversion state does not exist
          BLOG(3, "Successfully migrated conversion state");

          AdsClientHelper::Get()->SetBooleanPref(
              prefs::kHasMigratedConversionState, true);

          callback(/* success */ true);
          return;
        }

        const absl::optional<ConversionQueueItemList> conversion_queue_items =
            FromJson(json);

        if (!conversion_queue_items) {
          BLOG(0, "Failed to migrate conversions");
          callback(/* success */ false);
          return;
        }

        BLOG(3, "Successfully loaded conversion state");

        BLOG(1, "Migrating conversion state");

        database::table::ConversionQueue conversion_queue;
        conversion_queue.Save(
            conversion_queue_items.value(), [=](const bool success) {
              if (!success) {
                BLOG(0, "Failed to migrate conversion state");
                callback(/* success */ false);
                return;
              }

              AdsClientHelper::Get()->SetBooleanPref(
                  prefs::kHasMigratedConversionState, true);

              BLOG(3, "Successfully migrated conversion state");
              callback(/* success */ true);
            });
      });
}

}  // namespace conversions
}  // namespace ads

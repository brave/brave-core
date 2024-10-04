/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/operational_patterns_util.h"

#include <string_view>

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "base/values.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"

namespace brave_federated {

namespace {
constexpr char kWikiUrl[] =
    "https://github.com/brave/brave-browser/wiki/Operational-Patterns";
}  // namespace

int GetCollectionSlot() {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);

  const int month_to_date_in_seconds =
      (now.day_of_month - 1) * base::Time::kHoursPerDay *
      base::Time::kMinutesPerHour * base::Time::kSecondsPerMinute;
  const int seconds_today =
      now.hour * base::Time::kMinutesPerHour * base::Time::kSecondsPerMinute +
      now.minute * base::Time::kSecondsPerMinute + now.second;
  const int slot_size_in_seconds =
      brave_federated::features::GetCollectionSlotSizeInSeconds();
  return (month_to_date_in_seconds + seconds_today) / slot_size_in_seconds;
}

std::string CreateCollectionId() {
  return base::ToUpperASCII(base::UnguessableToken::Create().ToString());
}

std::string BuildCollectionPingPayload(std::string_view collection_id,
                                       int slot) {
  base::Value::Dict root;
  root.Set("collection_id", collection_id);
  root.Set("platform", brave_stats::GetPlatformIdentifier());
  root.Set("collection_slot", slot);
  root.Set("wiki-link", kWikiUrl);

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

std::string BuildDeletePingPayload(std::string_view collection_id) {
  base::Value::Dict root;
  root.Set("collection_id", collection_id);
  root.Set("wiki-link", kWikiUrl);

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

bool ShouldResetCollectionId(std::string_view collection_id,
                             const base::Time expiration_time) {
  if (collection_id.empty() || expiration_time.is_null()) {
    return true;
  }

  if (base::Time::Now() > expiration_time) {
    return true;
  }

  return false;
}

}  // namespace brave_federated

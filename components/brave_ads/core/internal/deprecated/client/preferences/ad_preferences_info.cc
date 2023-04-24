/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/ad_preferences_info.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

AdPreferencesInfo::AdPreferencesInfo() = default;

AdPreferencesInfo::AdPreferencesInfo(const AdPreferencesInfo& other) = default;

AdPreferencesInfo& AdPreferencesInfo::operator=(
    const AdPreferencesInfo& other) = default;

AdPreferencesInfo::AdPreferencesInfo(AdPreferencesInfo&& other) noexcept =
    default;

AdPreferencesInfo& AdPreferencesInfo::operator=(
    AdPreferencesInfo&& other) noexcept = default;

AdPreferencesInfo::~AdPreferencesInfo() = default;

base::Value::Dict AdPreferencesInfo::ToValue() const {
  base::Value::Dict dict;

  base::Value::List advertisers_list;
  for (const auto& advertiser : filtered_advertisers) {
    base::Value::Dict advertiser_dict;
    advertiser_dict.Set("id", advertiser.id);
    advertisers_list.Append(std::move(advertiser_dict));
  }
  dict.Set("filtered_advertisers", std::move(advertisers_list));

  base::Value::List categories_list;
  for (const auto& category : filtered_categories) {
    base::Value::Dict category_dict;
    category_dict.Set("name", category.name);
    categories_list.Append(std::move(category_dict));
  }
  dict.Set("filtered_categories", std::move(categories_list));

  base::Value::List saved_ads_list;
  for (const auto& saved_ad : saved_ads) {
    base::Value::Dict saved_ad_dict;
    saved_ad_dict.Set("creative_instance_id", saved_ad.creative_instance_id);
    saved_ads_list.Append(std::move(saved_ad_dict));
  }
  dict.Set("saved_ads", std::move(saved_ads_list));

  base::Value::List flagged_ads_list;
  for (const auto& flagged_ad : flagged_ads) {
    base::Value::Dict flagged_ad_dict;
    flagged_ad_dict.Set("creative_set_id", flagged_ad.creative_set_id);
    flagged_ads_list.Append(std::move(flagged_ad_dict));
  }
  dict.Set("flagged_ads", std::move(flagged_ads_list));

  return dict;
}

// TODO(https://github.com/brave/brave-browser/issues/24939): Reduce cognitive
// complexity.
void AdPreferencesInfo::FromValue(const base::Value::Dict& dict) {
  if (const auto* const value = dict.FindList("filtered_advertisers")) {
    for (const auto& item : *value) {
      if (!item.is_dict()) {
        continue;
      }

      const auto& advertiser = item.GetDict();
      const auto* const id = advertiser.FindString("id");
      if (!id) {
        continue;
      }

      FilteredAdvertiserInfo filtered_advertiser;
      filtered_advertiser.id = *id;
      filtered_advertisers.push_back(filtered_advertiser);
    }
  }

  if (const auto* const value = dict.FindList("filtered_categories")) {
    for (const auto& item : *value) {
      if (!item.is_dict()) {
        continue;
      }

      const auto& category = item.GetDict();
      const auto* const name = category.FindString("name");
      if (!name) {
        continue;
      }

      FilteredCategoryInfo filtered_category;
      filtered_category.name = *name;
      filtered_categories.push_back(filtered_category);
    }
  }

  if (const auto* const value = dict.FindList("saved_ads")) {
    for (const auto& item : *value) {
      if (!item.is_dict()) {
        continue;
      }

      const auto& ad = item.GetDict();
      const auto* const creative_instance_id =
          ad.FindString("creative_instance_id");
      if (!creative_instance_id) {
        continue;
      }

      SavedAdInfo saved_ad;
      saved_ad.creative_instance_id = *creative_instance_id;
      saved_ads.push_back(saved_ad);
    }
  }

  if (const auto* const value = dict.FindList("flagged_ads")) {
    for (const auto& item : *value) {
      if (!item.is_dict()) {
        continue;
      }

      const auto& ad = item.GetDict();
      const auto* const creative_set_id = ad.FindString("creative_set_id");
      if (!creative_set_id) {
        continue;
      }

      FlaggedAdInfo flagged_ad;
      flagged_ad.creative_set_id = *creative_set_id;
      flagged_ads.push_back(flagged_ad);
    }
  }
}

std::string AdPreferencesInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool AdPreferencesInfo::FromJson(const std::string& json) {
  const absl::optional<base::Value> root =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!root || !root->is_dict()) {
    return false;
  }

  FromValue(root->GetDict());

  return true;
}

}  // namespace brave_ads

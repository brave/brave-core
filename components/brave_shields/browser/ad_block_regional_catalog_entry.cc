/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_catalog_entry.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

bool GetStringVector(const base::Value* value,
                     std::vector<std::string>* field) {
  field->clear();
  if (value == nullptr || !value->is_list()) {
    return false;
  } else {
    const base::Value::List& list = value->GetList();
    for (const auto& list_value : list) {
      const auto* s = list_value.GetIfString();
      if (s) {
        field->push_back(*s);
      } else {
        return false;
      }
    }
    return true;
  }
}

}  // namespace

namespace brave_shields {

RegionalCatalogEntry::RegionalCatalogEntry() {}

RegionalCatalogEntry::RegionalCatalogEntry(
    const std::string& uuid,
    const std::string& url,
    const std::string& title,
    const std::vector<std::string>& langs,
    const std::string& support_url,
    const std::string& component_id,
    const std::string& base64_public_key,
    const std::string& desc)
    : uuid(uuid),
      url(url),
      title(title),
      langs(langs),
      support_url(support_url),
      component_id(component_id),
      base64_public_key(base64_public_key),
      desc(desc) {}

RegionalCatalogEntry::RegionalCatalogEntry(const RegionalCatalogEntry& other) =
    default;

RegionalCatalogEntry::~RegionalCatalogEntry() = default;

void RegionalCatalogEntry::RegisterJSONConverter(
    base::JSONValueConverter<RegionalCatalogEntry>* converter) {
  converter->RegisterStringField("uuid", &RegionalCatalogEntry::uuid);
  converter->RegisterStringField("url", &RegionalCatalogEntry::url);
  converter->RegisterStringField("title", &RegionalCatalogEntry::title);
  converter->RegisterCustomValueField<std::vector<std::string>>(
      "langs", &RegionalCatalogEntry::langs, &GetStringVector);
  converter->RegisterStringField("support_url",
                                 &RegionalCatalogEntry::support_url);
  converter->RegisterStringField("component_id",
                                 &RegionalCatalogEntry::component_id);
  converter->RegisterStringField("base64_public_key",
                                 &RegionalCatalogEntry::base64_public_key);
  converter->RegisterStringField("desc", &RegionalCatalogEntry::desc);
}

std::vector<RegionalCatalogEntry>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<RegionalCatalogEntry>& region_lists,
    const std::string& uuid) {
  std::string uuid_uppercase = base::ToUpperASCII(uuid);
  return base::ranges::find(region_lists, uuid_uppercase,
                            &RegionalCatalogEntry::uuid);
}

std::vector<RegionalCatalogEntry>::const_iterator FindAdBlockFilterListByLocale(
    const std::vector<RegionalCatalogEntry>& region_lists,
    const std::string& locale) {
  std::string adjusted_locale;
  std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    adjusted_locale = locale;
  } else {
    adjusted_locale = locale.substr(0, loc);
  }
  adjusted_locale = base::ToLowerASCII(adjusted_locale);
  return base::ranges::find_if(
      region_lists, [&adjusted_locale](const RegionalCatalogEntry& entry) {
        return base::Contains(entry.langs, adjusted_locale);
      });
}

std::vector<RegionalCatalogEntry> RegionalCatalogFromJSON(
    const std::string& catalog_json) {
  std::vector<RegionalCatalogEntry> catalog =
      std::vector<RegionalCatalogEntry>();

  absl::optional<base::Value> parsed_json =
      base::JSONReader::Read(catalog_json);
  if (!parsed_json || !parsed_json->is_list()) {
    LOG(ERROR) << "Could not load regional adblock catalog";
    return catalog;
  }

  base::JSONValueConverter<RegionalCatalogEntry> converter;

  base::Value::List& regional_lists = parsed_json->GetList();
  for (const auto& item : regional_lists) {
    DCHECK(item.is_dict());
    RegionalCatalogEntry entry;
    converter.Convert(item, &entry);
    catalog.push_back(entry);
  }

  return catalog;
}

}  // namespace brave_shields

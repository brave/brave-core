// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace {

bool GetComponentId(const base::Value* value, std::string* field) {
  if (value == nullptr || !value->is_dict()) {
    return false;
  } else {
    const base::Value::Dict& dict = value->GetDict();
    const auto* component_id = dict.FindString("component_id");
    if (component_id) {
      *field = *component_id;
      return true;
    } else {
      return false;
    }
  }
}

bool GetBase64PublicKey(const base::Value* value, std::string* field) {
  if (value == nullptr || !value->is_dict()) {
    return false;
  } else {
    const base::Value::Dict& dict = value->GetDict();
    const auto* component_id = dict.FindString("base64_public_key");
    if (component_id) {
      *field = *component_id;
      return true;
    } else {
      return false;
    }
  }
}

bool GetStringVector(const base::Value* value,
                     std::vector<std::string>* field) {
  DCHECK(field);
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

bool GetUint8(const base::Value* value, uint8_t* field) {
  DCHECK(field);
  if (value == nullptr || !value->is_int()) {
    return false;
  } else {
    int i = value->GetInt();
    if (!base::IsValueInRangeForNumericType<uint8_t>(i)) {
      return false;
    }
    *field = base::checked_cast<uint8_t>(i);
    return true;
  }
}

#if BUILDFLAG(IS_LINUX)
constexpr char kCurrentPlatform[] = "LINUX";
#elif BUILDFLAG(IS_WIN)
constexpr char kCurrentPlatform[] = "WINDOWS";
#elif BUILDFLAG(IS_MAC)
constexpr char kCurrentPlatform[] = "MAC";
#elif BUILDFLAG(IS_ANDROID)
constexpr char kCurrentPlatform[] = "ANDROID";
#elif BUILDFLAG(IS_IOS)
constexpr char kCurrentPlatform[] = "IOS";
#else
constexpr char kCurrentPlatform[] = "OTHER";
#endif

}  // namespace

namespace brave_shields {

FilterListCatalogEntry::FilterListCatalogEntry() {}

FilterListCatalogEntry::FilterListCatalogEntry(
    const std::string& uuid,
    const std::string& url,
    const std::string& title,
    const std::vector<std::string>& langs,
    const std::string& support_url,
    const std::string& desc,
    bool hidden,
    bool default_enabled,
    bool first_party_protections,
    uint8_t permission_mask,
    const std::vector<std::string>& platforms,
    const std::string& component_id,
    const std::string& base64_public_key)
    : uuid(uuid),
      url(url),
      title(title),
      langs(langs),
      support_url(support_url),
      desc(desc),
      hidden(hidden),
      default_enabled(default_enabled),
      first_party_protections(first_party_protections),
      permission_mask(permission_mask),
      platforms(platforms),
      component_id(component_id),
      base64_public_key(base64_public_key) {}

FilterListCatalogEntry::FilterListCatalogEntry(
    const FilterListCatalogEntry& other) = default;

FilterListCatalogEntry::~FilterListCatalogEntry() = default;

void FilterListCatalogEntry::RegisterJSONConverter(
    base::JSONValueConverter<FilterListCatalogEntry>* converter) {
  converter->RegisterStringField("uuid", &FilterListCatalogEntry::uuid);
  converter->RegisterStringField("url", &FilterListCatalogEntry::url);
  converter->RegisterStringField("title", &FilterListCatalogEntry::title);
  converter->RegisterCustomValueField<std::vector<std::string>>(
      "langs", &FilterListCatalogEntry::langs, &GetStringVector);
  converter->RegisterStringField("support_url",
                                 &FilterListCatalogEntry::support_url);
  converter->RegisterStringField("desc", &FilterListCatalogEntry::desc);
  converter->RegisterBoolField("hidden", &FilterListCatalogEntry::hidden);
  converter->RegisterBoolField("default_enabled",
                               &FilterListCatalogEntry::default_enabled);
  converter->RegisterBoolField(
      "first_party_protections",
      &FilterListCatalogEntry::first_party_protections);
  converter->RegisterCustomValueField(
      "permission_mask", &FilterListCatalogEntry::permission_mask, &GetUint8);
  converter->RegisterCustomValueField("list_text_component",
                                      &FilterListCatalogEntry::component_id,
                                      &GetComponentId);
  converter->RegisterCustomValueField(
      "list_text_component", &FilterListCatalogEntry::base64_public_key,
      &GetBase64PublicKey);
  converter->RegisterCustomValueField<std::vector<std::string>>(
      "platforms", &FilterListCatalogEntry::platforms, &GetStringVector);
}

bool FilterListCatalogEntry::SupportsCurrentPlatform() const {
  if (platforms.empty()) {
    return true;
  }

  return std::find(platforms.begin(), platforms.end(), kCurrentPlatform) !=
         platforms.end();
}

std::vector<FilterListCatalogEntry>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& uuid) {
  return base::ranges::find(region_lists, uuid, &FilterListCatalogEntry::uuid);
}

// Given a locale like `en-US`, find regional lists corresponding to the
// language (`en`) part.
std::vector<std::reference_wrapper<FilterListCatalogEntry const>>
FindAdBlockFilterListsByLocale(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& locale) {
  std::vector<std::reference_wrapper<FilterListCatalogEntry const>> output;

  std::string adjusted_locale;
  std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    adjusted_locale = locale;
  } else {
    adjusted_locale = locale.substr(0, loc);
  }

  adjusted_locale = base::ToLowerASCII(adjusted_locale);
  std::copy_if(region_lists.begin(), region_lists.end(),
               std::back_inserter(output),
               [&adjusted_locale](const FilterListCatalogEntry& entry) {
                 return base::Contains(entry.langs, adjusted_locale);
               });

  return output;
}

std::vector<FilterListCatalogEntry> FilterListCatalogFromJSON(
    const std::string& catalog_json) {
  std::vector<FilterListCatalogEntry> catalog =
      std::vector<FilterListCatalogEntry>();

  std::optional<base::Value> parsed_json = base::JSONReader::Read(catalog_json);
  if (!parsed_json || !parsed_json->is_list()) {
    LOG(ERROR) << "Could not load regional adblock catalog";
    return catalog;
  }

  base::JSONValueConverter<FilterListCatalogEntry> converter;

  base::Value::List& regional_lists = parsed_json->GetList();
  for (const auto& item : regional_lists) {
    DCHECK(item.is_dict());
    FilterListCatalogEntry entry;
    converter.Convert(item, &entry);
    catalog.push_back(entry);
  }

  return catalog;
}

}  // namespace brave_shields

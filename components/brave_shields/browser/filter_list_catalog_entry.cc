/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/filter_list_catalog_entry.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
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

}  // namespace

namespace brave_shields {

FilterListCatalogEntry::FilterListCatalogEntry() {}

FilterListCatalogEntry::FilterListCatalogEntry(
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
  converter->RegisterCustomValueField("list_text_component",
                                      &FilterListCatalogEntry::component_id,
                                      &GetComponentId);
  converter->RegisterCustomValueField(
      "list_text_component", &FilterListCatalogEntry::base64_public_key,
      &GetBase64PublicKey);
  converter->RegisterStringField("desc", &FilterListCatalogEntry::desc);
}

std::vector<FilterListCatalogEntry>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& uuid) {
  std::string uuid_uppercase = base::ToUpperASCII(uuid);
  return base::ranges::find(region_lists, uuid_uppercase,
                            &FilterListCatalogEntry::uuid);
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

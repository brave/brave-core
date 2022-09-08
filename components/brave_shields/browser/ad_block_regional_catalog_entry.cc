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
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_shields {

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

  base::Value::List& regional_lists = parsed_json->GetList();
  for (const auto& item : regional_lists) {
    DCHECK(item.is_dict());
    const base::Value::Dict& regional_list = item.GetDict();

    const auto* uuid = regional_list.FindString("uuid");
    if (!uuid) {
      continue;
    }
    const auto* url = regional_list.FindString("url");
    if (!url) {
      continue;
    }
    const auto* title = regional_list.FindString("title");
    if (!title) {
      continue;
    }
    std::vector<std::string> langs = std::vector<std::string>();
    const auto* langs_key = regional_list.FindList("langs");
    if (!langs_key) {
      continue;
    }
    for (const auto& lang : *langs_key) {
      DCHECK(lang.is_string());
      langs.push_back(lang.GetString());
    }
    const auto* support_url = regional_list.FindString("support_url");
    if (!support_url) {
      continue;
    }
    const auto* component_id = regional_list.FindString("component_id");
    if (!component_id) {
      continue;
    }
    const auto* base64_public_key =
        regional_list.FindString("base64_public_key");
    if (!base64_public_key) {
      continue;
    }
    const auto* desc = regional_list.FindString("desc");
    if (!desc) {
      continue;
    }

    catalog.emplace_back(*uuid, *url, *title, langs, *support_url,
                         *component_id, *base64_public_key, *desc);
  }

  return catalog;
}

}  // namespace brave_shields

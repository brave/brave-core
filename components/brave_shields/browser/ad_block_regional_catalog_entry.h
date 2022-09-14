/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_CATALOG_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_CATALOG_ENTRY_H_

#include <string>
#include <vector>

namespace base {
template <typename StructType>
class JSONValueConverter;
}

namespace brave_shields {

class RegionalCatalogEntry {
 public:
  RegionalCatalogEntry();
  RegionalCatalogEntry(const std::string& uuid,
                       const std::string& url,
                       const std::string& title,
                       const std::vector<std::string>& langs,
                       const std::string& support_url,
                       const std::string& component_id,
                       const std::string& base64_public_key,
                       const std::string& desc);
  explicit RegionalCatalogEntry(const RegionalCatalogEntry& other);
  ~RegionalCatalogEntry();

  std::string uuid;
  std::string url;
  std::string title;
  std::vector<std::string> langs;
  std::string support_url;
  std::string component_id;
  std::string base64_public_key;
  std::string desc;

  static void RegisterJSONConverter(
      base::JSONValueConverter<RegionalCatalogEntry>*);
};

std::vector<RegionalCatalogEntry>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<RegionalCatalogEntry>& region_lists,
    const std::string& uuid);
std::vector<RegionalCatalogEntry>::const_iterator FindAdBlockFilterListByLocale(
    const std::vector<RegionalCatalogEntry>& region_lists,
    const std::string& locale);

std::vector<RegionalCatalogEntry> RegionalCatalogFromJSON(
    const std::string& catalog_json);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_CATALOG_ENTRY_H_

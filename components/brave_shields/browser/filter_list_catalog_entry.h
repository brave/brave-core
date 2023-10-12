/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_FILTER_LIST_CATALOG_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_FILTER_LIST_CATALOG_ENTRY_H_

#include <functional>
#include <string>
#include <vector>

namespace base {
template <typename StructType>
class JSONValueConverter;
}

namespace brave_shields {

// Represents an entry of the catalog of filter lists that Brave makes
// available. Corresponds to the entries in
// https://github.com/brave/adblock-resources/blob/master/filter_lists/regional.json
class FilterListCatalogEntry {
 public:
  FilterListCatalogEntry();
  FilterListCatalogEntry(const std::string& uuid,
                         const std::string& url,
                         const std::string& title,
                         const std::vector<std::string>& langs,
                         const std::string& support_url,
                         const std::string& component_id,
                         const std::string& base64_public_key,
                         const std::string& desc);
  explicit FilterListCatalogEntry(const FilterListCatalogEntry& other);
  ~FilterListCatalogEntry();

  std::string uuid;
  std::string url;
  std::string title;
  std::vector<std::string> langs;
  std::string support_url;
  std::string component_id;
  std::string base64_public_key;
  std::string desc;

  static void RegisterJSONConverter(
      base::JSONValueConverter<FilterListCatalogEntry>*);
};

std::vector<FilterListCatalogEntry>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& uuid);
std::vector<std::reference_wrapper<FilterListCatalogEntry const>>
FindAdBlockFilterListsByLocale(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& locale);

std::vector<FilterListCatalogEntry> FilterListCatalogFromJSON(
    const std::string& catalog_json);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_FILTER_LIST_CATALOG_ENTRY_H_

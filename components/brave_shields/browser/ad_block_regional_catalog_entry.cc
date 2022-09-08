/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_catalog_entry.h"

#include <string>
#include <vector>

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

}  // namespace brave_shields

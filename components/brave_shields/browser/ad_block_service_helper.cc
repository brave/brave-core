/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service_helper.h"

#include <algorithm>

#include "base/strings/string_util.h"

namespace brave_shields {

std::vector<FilterList>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterList>& region_lists,
    const std::string& uuid) {
  std::string uuid_uppercase = base::ToUpperASCII(uuid);
  return std::find_if(region_lists.begin(), region_lists.end(),
                      [&uuid_uppercase](const FilterList& filter_list) {
                        return filter_list.uuid == uuid_uppercase;
                      });
}

std::vector<FilterList>::const_iterator FindAdBlockFilterListByLocale(
    const std::vector<FilterList>& region_lists,
    const std::string& locale) {
  std::string adjusted_locale;
  std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    adjusted_locale = locale;
  } else {
    adjusted_locale = locale.substr(0, loc);
  }
  adjusted_locale = base::ToLowerASCII(adjusted_locale);
  return std::find_if(
      region_lists.begin(), region_lists.end(),
      [&adjusted_locale](const FilterList& filter_list) {
        return std::find_if(filter_list.langs.begin(), filter_list.langs.end(),
                            [adjusted_locale](const std::string& lang) {
                              return lang == adjusted_locale;
                            }) != filter_list.langs.end();
      });
}

}  // namespace brave_shields

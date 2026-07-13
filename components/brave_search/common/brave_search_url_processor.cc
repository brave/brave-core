// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/brave_search_url_processor.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/brave_search/common/brave_search_country.h"

namespace brave_search {

std::string ProcessBraveSearchUrl(const std::string& url, PrefService* prefs) {
  // Check if the URL contains our custom placeholder.
  size_t placeholder_pos = url.find(kBraveCountryPlaceholder);
  if (placeholder_pos == std::string::npos) {
    return url;
  }

  std::string country_code = GetBraveSearchCountryCode(prefs);

  if (!country_code.empty()) {
    // Replace the placeholder with the actual country code.
    std::string result = url;
    base::ReplaceSubstringsAfterOffset(&result, 0, kBraveCountryPlaceholder,
                                       country_code);
    return result;
  }

  // Country code could not be determined. Remove the entire &country=
  // parameter to let the Brave Search server use its own detection.
  std::string result = url;
  std::string full_param =
      std::string("&country=") + kBraveCountryPlaceholder;
  base::ReplaceSubstringsAfterOffset(&result, 0, full_param, "");
  return result;
}

}  // namespace brave_search

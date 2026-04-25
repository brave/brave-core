/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_parser.h"

#include <iomanip>
#include <sstream>

#include "base/strings/string_util.h"

namespace brave_ads {

KeywordList ParseKeyphrase(const std::string& keyphrase) {
  std::istringstream iss(base::ToLowerASCII(keyphrase));

  KeywordList keywords;

  std::string keyword;
  while (iss >> std::quoted(keyword)) {
    keywords.emplace_back(
        base::TrimString(keyword, " ", base::TrimPositions::TRIM_ALL));
  }

  return keywords;
}

}  // namespace brave_ads

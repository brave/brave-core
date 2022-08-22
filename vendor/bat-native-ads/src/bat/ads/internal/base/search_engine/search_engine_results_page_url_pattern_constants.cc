/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine/search_engine_results_page_url_pattern_constants.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "bat/ads/internal/base/search_engine/search_engine_url_pattern_constants.h"

namespace ads {

const std::string& GetAmazonResultsPageUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(
      base::StrCat({GetAmazonUrlPattern(), "s"}));
  return *url_pattern;
}

const std::string& GetGoogleResultsPageUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(
      base::StrCat({GetGoogleUrlPattern(), "search"}));
  return *url_pattern;
}

const std::string& GetMojeekResultsPageUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(
      base::StrCat({GetMojeekUrlPattern(), "search"}));
  return *url_pattern;
}

const std::string& GetWikipediaResultsPageUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(
      base::StrCat({GetWikipediaUrlPattern(), "wiki/(.*)"}));
  return *url_pattern;
}

const std::string& GetYahooResultsPageUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(
      base::StrCat({GetYahooUrlPattern(), "search(.*)"}));
  return *url_pattern;
}

}  // namespace ads

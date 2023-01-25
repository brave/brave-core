/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_results_page_url_pattern_constants.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "bat/ads/internal/common/search_engine/search_engine_url_pattern_constants.h"

namespace ads {

const std::string& GetAmazonResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      base::StrCat({GetAmazonUrlPattern(), "s"}));
  return *kUrlPattern;
}

const std::string& GetGoogleResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      base::StrCat({GetGoogleUrlPattern(), "search"}));
  return *kUrlPattern;
}

const std::string& GetMojeekResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      base::StrCat({GetMojeekUrlPattern(), "search"}));
  return *kUrlPattern;
}

const std::string& GetWikipediaResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      base::StrCat({GetWikipediaUrlPattern(), "wiki/(.*)"}));
  return *kUrlPattern;
}

const std::string& GetYahooResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      base::StrCat({GetYahooUrlPattern(), "search(.*)"}));
  return *kUrlPattern;
}

}  // namespace ads

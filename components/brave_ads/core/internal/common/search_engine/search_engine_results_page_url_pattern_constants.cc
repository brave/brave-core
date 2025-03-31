/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_url_pattern_constants.h"

#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_url_pattern_constants.h"

namespace brave_ads {

const std::string& GetAmazonResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      GetAmazonUrlPattern() + "s");
  return *kUrlPattern;
}

const std::string& GetGoogleResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      GetGoogleUrlPattern() + "search");
  return *kUrlPattern;
}

const std::string& GetMojeekResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      GetMojeekUrlPattern() + "search");
  return *kUrlPattern;
}

const std::string& GetWikipediaResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      GetWikipediaUrlPattern() + "wiki/(.*)");
  return *kUrlPattern;
}

const std::string& GetYahooResultsPageUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(
      GetYahooUrlPattern() + "search(.*)");
  return *kUrlPattern;
}

}  // namespace brave_ads

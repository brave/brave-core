/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine/search_engine_url_pattern_util.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/base/search_engine/search_engine_domain_extensions_util.h"
#include "bat/ads/internal/base/search_engine/search_engine_subdomains_util.h"

namespace ads {

const std::string& GetAmazonUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(base::StrCat(
      {"https://www.amazon.(",
       base::JoinString(GetAmazonSearchEngineDomainExtensions(), "|"), ")/"}));
  return *url_pattern;
}

const std::string& GetGoogleUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(base::StrCat(
      {"https://www.google.(",
       base::JoinString(GetGoogleSearchEngineDomainExtensions(), "|"), ")/"}));
  return *url_pattern;
}

const std::string& GetMojeekUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(base::StrCat(
      {"https://www.mojeek.(",
       base::JoinString(GetMojeekSearchEngineDomainExtensions(), "|"), ")/"}));
  return *url_pattern;
}

const std::string& GetWikipediaUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(base::StrCat(
      {"https://(", base::JoinString(GetWikipediaSearchEngineSubdomains(), "|"),
       ").wikipedia.org/"}));
  return *url_pattern;
}

const std::string& GetYahooUrlPattern() {
  static base::NoDestructor<std::string> url_pattern(base::StrCat(
      {"https://((", base::JoinString(GetYahooSearchEngineSubdomains(), "|"),
       ").search.yahoo.com/|search.yahoo.com/)"}));
  return *url_pattern;
}

}  // namespace ads

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_url_pattern_constants.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_domain_extension_constants.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_subdomain_constants.h"

namespace brave_ads {

const std::string& GetAmazonUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(base::StrCat(
      {"https://www.amazon.(",
       base::JoinString(GetAmazonSearchEngineDomainExtensions(), "|"), ")/"}));
  return *kUrlPattern;
}

const std::string& GetGoogleUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(base::StrCat(
      {"https://www.google.(",
       base::JoinString(GetGoogleSearchEngineDomainExtensions(), "|"), ")/"}));
  return *kUrlPattern;
}

const std::string& GetMojeekUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(base::StrCat(
      {"https://www.mojeek.(",
       base::JoinString(GetMojeekSearchEngineDomainExtensions(), "|"), ")/"}));
  return *kUrlPattern;
}

const std::string& GetWikipediaUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(base::StrCat(
      {"https://(", base::JoinString(GetWikipediaSearchEngineSubdomains(), "|"),
       ").wikipedia.org/"}));
  return *kUrlPattern;
}

const std::string& GetYahooUrlPattern() {
  static const base::NoDestructor<std::string> kUrlPattern(base::StrCat(
      {"https://((", base::JoinString(GetYahooSearchEngineSubdomains(), "|"),
       ").search.yahoo.com/|search.yahoo.com/)"}));
  return *kUrlPattern;
}

}  // namespace brave_ads

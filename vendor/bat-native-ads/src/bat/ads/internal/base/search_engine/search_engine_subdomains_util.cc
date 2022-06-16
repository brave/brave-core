/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine/search_engine_subdomains_util.h"

#include "base/no_destructor.h"

namespace ads {

const std::vector<std::string>& GetWikipediaSearchEngineSubdomains() {
  // See https://www.wikipedia.org/.
  static base::NoDestructor<std::vector<std::string>> extensions(
      {"af", "ar", "arz", "ast", "az",  "azb", "be", "bg",    "bn",
       "ca", "ce", "ceb", "cs",  "cy",  "da",  "de", "el",    "en",
       "eo", "es", "et",  "eu",  "fa",  "fi",  "fr", "gl",    "he",
       "hi", "hr", "hu",  "hy",  "id",  "it",  "ja", "ka",    "kk",
       "ko", "la", "lt",  "lv",  "min", "mk",  "ms", "my",    "nan",
       "nl", "nn", "no",  "pl",  "pt",  "ro",  "ru", "sh",    "simple",
       "sk", "sl", "sr",  "sv",  "ta",  "tg",  "th", "tr",    "tt",
       "uk", "ur", "uz",  "vi",  "vo",  "war", "zh", "zh-yue"});
  return *extensions;
}

const std::vector<std::string>& GetYahooSearchEngineSubdomains() {
  // See https://uk.yahoo.com/everything/world.
  static base::NoDestructor<std::vector<std::string>> extensions(
      {"au",      "be", "br",       "ca", "de", "en-maktoob", "es",
       "espanol", "fr", "fr-be",    "gr", "hk", "id",         "ie",
       "in",      "it", "malaysia", "nz", "ph", "qc",         "ro",
       "se",      "sg", "tw",       "uk", "vn", "www",        "za"});
  return *extensions;
}

}  // namespace ads

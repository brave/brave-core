/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_subdomain_constants.h"

namespace ads {

base::span<const base::StringPiece> GetWikipediaSearchEngineSubdomains() {
  // See https://www.wikipedia.org/.
  static constexpr base::StringPiece kExtensions[] = {
      "af", "ar", "arz", "ast", "az",  "azb", "be", "bg",    "bn",
      "ca", "ce", "ceb", "cs",  "cy",  "da",  "de", "el",    "en",
      "eo", "es", "et",  "eu",  "fa",  "fi",  "fr", "gl",    "he",
      "hi", "hr", "hu",  "hy",  "id",  "it",  "ja", "ka",    "kk",
      "ko", "la", "lt",  "lv",  "min", "mk",  "ms", "my",    "nan",
      "nl", "nn", "no",  "pl",  "pt",  "ro",  "ru", "sh",    "simple",
      "sk", "sl", "sr",  "sv",  "ta",  "tg",  "th", "tr",    "tt",
      "uk", "ur", "uz",  "vi",  "vo",  "war", "zh", "zh-yue"};

  return kExtensions;
}

base::span<const base::StringPiece> GetYahooSearchEngineSubdomains() {
  // See https://uk.yahoo.com/everything/world.
  static constexpr base::StringPiece kExtensions[] = {
      "au",      "be", "br",       "ca", "de", "en-maktoob", "es",
      "espanol", "fr", "fr-be",    "gr", "hk", "id",         "ie",
      "in",      "it", "malaysia", "nz", "ph", "qc",         "ro",
      "se",      "sg", "tw",       "uk", "vn", "www",        "za"};

  return kExtensions;
}

}  // namespace ads

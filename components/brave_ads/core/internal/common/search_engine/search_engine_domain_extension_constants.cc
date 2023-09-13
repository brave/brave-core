/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_domain_extension_constants.h"

namespace brave_ads {

base::span<const std::string_view> GetAmazonSearchEngineDomainExtensions() {
  // See https://www.amazon.com/gp/navigation-country/select-country.
  static constexpr std::string_view kExtensions[] = {
      "ae",     "ca",     "cn", "co.jp", "co.uk", "com", "com.au",
      "com.br", "com.mx", "de", "eg",    "es",    "fr",  "in",
      "it",     "nl",     "pl", "sa",    "se",    "sp",  "tr"};

  return kExtensions;
}

base::span<const std::string_view> GetGoogleSearchEngineDomainExtensions() {
  static constexpr std::string_view kExtensions[] = {
      "ac",     "ad",     "ae",     "al",     "am",     "as",     "at",
      "az",     "ba",     "be",     "bf",     "bg",     "bi",     "bj",
      "bs",     "bt",     "ca",     "cat",    "cd",     "cf",     "cg",
      "ch",     "ci",     "ci",     "cl",     "cm",     "cn",     "co.bw",
      "co.ck",  "co.cr",  "co.id",  "co.il",  "co.im",  "co.in",  "co.je",
      "co.jp",  "co.ke",  "co.kr",  "co.ls",  "co.ma",  "co.mz",  "co.nz",
      "co.th",  "co.tz",  "co.ug",  "co.uk",  "co.uz",  "co.ve",  "co.vi",
      "co.za",  "co.zm",  "co.zw",  "com.af", "com.ag", "com.ai", "com.ar",
      "com.au", "com.bd", "com.bh", "com.bn", "com.bo", "com.br", "com.by",
      "com.bz", "com.co", "com.cu", "com.cy", "com.do", "com.ec", "com.eg",
      "com.et", "com.fj", "com.gh", "com.gi", "com.gt", "com.hk", "com.jm",
      "com.kg", "com.kh", "com.kw", "com.lb", "com.ly", "com.mt", "com.mx",
      "com.my", "com.na", "com.nf", "com.ng", "com.ni", "com.np", "com.om",
      "com.pa", "com.pe", "com.pg", "com.ph", "com.pk", "com.pr", "com.py",
      "com.qa", "com.sa", "com.sb", "com.sg", "com.sl", "com.sv", "com.tj",
      "com.tr", "com.tw", "com.ua", "com.uy", "com.vc", "com.vn", "com",
      "cv",     "cz",     "de",     "dj",     "dk",     "dm",     "dz",
      "ee",     "es",     "fi",     "fm",     "fr",     "ga",     "ge",
      "gg",     "gl",     "gm",     "gp",     "gr",     "gy",     "hn",
      "hr",     "ht",     "hu",     "ie",     "iq",     "is",     "it.ao",
      "it",     "jo",     "ki",     "kz",     "la",     "li",     "lk",
      "lt",     "lu",     "lv",     "md",     "me",     "mg",     "mk",
      "ml",     "mn",     "ms",     "mu",     "mv",     "mw",     "ne",
      "nl",     "no",     "nr",     "nu",     "pl",     "pn",     "ps",
      "pt",     "ro",     "rs",     "ru",     "rw",     "sc",     "se",
      "sh",     "si",     "sk",     "sm",     "sn",     "so",     "sr",
      "st",     "td",     "tg",     "tk",     "tl",     "tm",     "tn",
      "to",     "tt",     "vg",     "vu",     "ws"};

  return kExtensions;
}

base::span<const std::string_view> GetMojeekSearchEngineDomainExtensions() {
  static constexpr std::string_view kExtensions[] = {"co.uk", "com"};

  return kExtensions;
}

}  // namespace brave_ads

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_DOMAIN_EXTENSION_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_DOMAIN_EXTENSION_CONSTANTS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_tree.h"

namespace brave_ads {

// See https://www.amazon.com/gp/navigation-country/select-country.
inline constexpr auto kAmazonSearchEngineDomainExtensions =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {"ae",     "ca",     "cn", "co.jp", "co.uk", "com", "com.au",
         "com.br", "com.mx", "de", "eg",    "es",    "fr",  "in",
         "it",     "nl",     "pl", "sa",    "se",    "sp",  "tr"});

inline constexpr auto kGoogleSearchEngineDomainExtensions =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {"ac",     "ad",     "ae",     "al",     "am",     "as",     "at",
         "az",     "ba",     "be",     "bf",     "bg",     "bi",     "bj",
         "bs",     "bt",     "ca",     "cat",    "cd",     "cf",     "cg",
         "ch",     "ci",     "cl",     "cm",     "cn",     "co.bw",  "co.ck",
         "co.cr",  "co.id",  "co.il",  "co.im",  "co.in",  "co.je",  "co.jp",
         "co.ke",  "co.kr",  "co.ls",  "co.ma",  "co.mz",  "co.nz",  "co.th",
         "co.tz",  "co.ug",  "co.uk",  "co.uz",  "co.ve",  "co.vi",  "co.za",
         "co.zm",  "co.zw",  "com",    "com.af", "com.ag", "com.ai", "com.ar",
         "com.au", "com.bd", "com.bh", "com.bn", "com.bo", "com.br", "com.by",
         "com.bz", "com.co", "com.cu", "com.cy", "com.do", "com.ec", "com.eg",
         "com.et", "com.fj", "com.gh", "com.gi", "com.gt", "com.hk", "com.jm",
         "com.kg", "com.kh", "com.kw", "com.lb", "com.ly", "com.mt", "com.mx",
         "com.my", "com.na", "com.nf", "com.ng", "com.ni", "com.np", "com.om",
         "com.pa", "com.pe", "com.pg", "com.ph", "com.pk", "com.pr", "com.py",
         "com.qa", "com.sa", "com.sb", "com.sg", "com.sl", "com.sv", "com.tj",
         "com.tr", "com.tw", "com.ua", "com.uy", "com.vc", "com.vn", "cv",
         "cz",     "de",     "dj",     "dk",     "dm",     "dz",     "ee",
         "es",     "fi",     "fm",     "fr",     "ga",     "ge",     "gg",
         "gl",     "gm",     "gp",     "gr",     "gy",     "hn",     "hr",
         "ht",     "hu",     "ie",     "iq",     "is",     "it",     "it.ao",
         "jo",     "ki",     "kz",     "la",     "li",     "lk",     "lt",
         "lu",     "lv",     "md",     "me",     "mg",     "mk",     "ml",
         "mn",     "ms",     "mu",     "mv",     "mw",     "ne",     "nl",
         "no",     "nr",     "nu",     "pl",     "pn",     "ps",     "pt",
         "ro",     "rs",     "ru",     "rw",     "sc",     "se",     "sh",
         "si",     "sk",     "sm",     "sn",     "so",     "sr",     "st",
         "td",     "tg",     "tk",     "tl",     "tm",     "tn",     "to",
         "tt",     "vg",     "vu",     "ws"});

inline constexpr auto kMojeekSearchEngineDomainExtensions =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {"co.uk", "com"});

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_DOMAIN_EXTENSION_CONSTANTS_H_

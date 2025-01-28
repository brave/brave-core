/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/backup_results_allowed_urls.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/url_constants.h"

namespace brave_search {

namespace {

constexpr auto kAllowedGoogleTLDs = base::MakeFixedFlatSet<std::string_view>({
    "ac",     "ad",     "ae",     "af",     "ag",     "al",     "am",
    "as",     "at",     "ax",     "az",     "ba",     "be",     "bf",
    "bg",     "bi",     "bj",     "bs",     "bt",     "by",     "ca",
    "cat",    "cc",     "cd",     "cf",     "cg",     "ch",     "ci",
    "cl",     "cm",     "cn",     "co.ao",  "co.bw",  "co.ck",  "co.cr",
    "co.hu",  "co.id",  "co.il",  "co.im",  "co.in",  "co.je",  "co.jp",
    "co.ke",  "co.kr",  "co.ls",  "com",    "co.ma",  "com.af", "com.ag",
    "com.ai", "com.ar", "com.au", "com.bd", "com.bh", "com.bn", "com.bo",
    "com.br", "com.by", "com.bz", "com.cn", "com.co", "com.cu", "com.cy",
    "com.do", "com.ec", "com.eg", "com.et", "com.fj", "com.ge", "com.gh",
    "com.gi", "com.gr", "com.gt", "com.hk", "com.iq", "com.jm", "com.jo",
    "com.kh", "com.kw", "com.lb", "com.ly", "com.mm", "com.mt", "com.mx",
    "com.my", "com.na", "com.nf", "com.ng", "com.ni", "com.np", "com.nr",
    "com.om", "com.pa", "com.pe", "com.pg", "com.ph", "com.pk", "com.pl",
    "com.pr", "com.py", "com.qa", "com.ru", "com.sa", "com.sb", "com.sg",
    "com.sl", "com.sv", "com.tj", "com.tn", "com.tr", "com.tw", "com.ua",
    "com.uy", "com.vc", "com.ve", "com.vn", "co.mz",  "co.nz",  "co.th",
    "co.tz",  "co.ug",  "co.uk",  "co.uz",  "co.ve",  "co.vi",  "co.za",
    "co.zm",  "co.zw",  "cv",     "cz",     "de",     "dj",     "dk",
    "dm",     "dz",     "ee",     "es",     "fi",     "fm",     "fr",
    "ga",     "ge",     "gg",     "gl",     "gm",     "gp",     "gr",
    "gy",     "hk",     "hn",     "hr",     "ht",     "hu",     "ie",
    "im",     "info",   "iq",     "is",     "it",     "it.ao",  "je",
    "jo",     "jobs",   "jp",     "kg",     "ki",     "kz",     "la",
    "li",     "lk",     "lt",     "lu",     "lv",     "md",     "me",
    "mg",     "mk",     "ml",     "mn",     "ms",     "mu",     "mv",
    "mw",     "ne",     "ne.jp",  "net",    "nl",     "no",     "nr",
    "nu",     "off.ai", "pk",     "pl",     "pn",     "ps",     "pt",
    "ro",     "rs",     "ru",     "rw",     "sc",     "se",     "sh",
    "si",     "sk",     "sm",     "sn",     "so",     "sr",     "st",
    "td",     "tg",     "tk",     "tl",     "tm",     "tn",     "to",
    "tt",     "ua",     "us",     "uz",     "vg",     "vu",     "ws",
});

constexpr char kGoogleSLD[] = "google";

}  // namespace

bool IsBackupResultURLAllowed(const GURL& url) {
  if (!url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  // Extract domain and registry using GetDomainAndRegistry
  auto domain_and_registry =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);

  if (domain_and_registry.empty()) {
    return false;
  }

  // Find the first dot position to split into SLD and TLD
  auto dot_pos = domain_and_registry.find('.');
  if (dot_pos == std::string::npos) {
    return false;
  }

  // Extract SLD and TLD
  auto sld = domain_and_registry.substr(0, dot_pos);
  auto tld = domain_and_registry.substr(dot_pos + 1);

  // Check if SLD is "google" and TLD is in allowed list
  return sld == kGoogleSLD && kAllowedGoogleTLDs.contains(tld);
}

}  // namespace brave_search

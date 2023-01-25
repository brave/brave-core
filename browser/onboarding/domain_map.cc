/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/onboarding/domain_map.h"

#include <sstream>

#include "base/containers/fixed_flat_map.h"
#include "base/strings/string_piece.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace domain_map {
constexpr auto kDomains =
    base::MakeFixedFlatMap<base::StringPiece, base::StringPiece>({
        {"2mdn.net", "Google"},
        {"admeld.com", "Google"},
        {"admob.com", "Google"},
        {"apture.com", "Google"},
        {"blogger.com", "Google"},
        {"cc-dt.com", "Google"},
        {"crashlytics.com", "Google"},
        {"destinationurl.com", "Google"},
        {"doubleclick.net", "Google"},
        {"ggpht.com", "Google"},
        {"gmail.com", "Google"},
        {"gmodules.com", "Google"},
        {"google-analytics.com", "Google"},
        {"google.ac", "Google"},
        {"google.ad", "Google"},
        {"google.ae", "Google"},
        {"google.al", "Google"},
        {"google.am", "Google"},
        {"google.as", "Google"},
        {"google.at", "Google"},
        {"google.az", "Google"},
        {"google.ba", "Google"},
        {"google.be", "Google"},
        {"google.bf", "Google"},
        {"google.bg", "Google"},
        {"google.bi", "Google"},
        {"google.bj", "Google"},
        {"google.bs", "Google"},
        {"google.bt", "Google"},
        {"google.by", "Google"},
        {"google.ca", "Google"},
        {"google.cat", "Google"},
        {"google.cc", "Google"},
        {"google.cd", "Google"},
        {"google.cf", "Google"},
        {"google.cg", "Google"},
        {"google.ch", "Google"},
        {"google.ci", "Google"},
        {"google.cl", "Google"},
        {"google.cm", "Google"},
        {"google.cn", "Google"},
        {"google.co.ao", "Google"},
        {"google.co.bw", "Google"},
        {"google.co.ck", "Google"},
        {"google.co.cr", "Google"},
        {"google.co.id", "Google"},
        {"google.co.il", "Google"},
        {"google.co.in", "Google"},
        {"google.co.jp", "Google"},
        {"google.co.ke", "Google"},
        {"google.co.kr", "Google"},
        {"google.co.ls", "Google"},
        {"google.co.ma", "Google"},
        {"google.co.mz", "Google"},
        {"google.co.nz", "Google"},
        {"google.co.th", "Google"},
        {"google.co.tz", "Google"},
        {"google.co.ug", "Google"},
        {"google.co.uk", "Google"},
        {"google.co.uz", "Google"},
        {"google.co.ve", "Google"},
        {"google.co.vi", "Google"},
        {"google.co.za", "Google"},
        {"google.co.zm", "Google"},
        {"google.co.zw", "Google"},
        {"google.com", "Google"},
        {"google.com.af", "Google"},
        {"google.com.ag", "Google"},
        {"google.com.ai", "Google"},
        {"google.com.ar", "Google"},
        {"google.com.au", "Google"},
        {"google.com.bd", "Google"},
        {"google.com.bh", "Google"},
        {"google.com.bn", "Google"},
        {"google.com.bo", "Google"},
        {"google.com.br", "Google"},
        {"google.com.bz", "Google"},
        {"google.com.co", "Google"},
        {"google.com.cu", "Google"},
        {"google.com.cy", "Google"},
        {"google.com.do", "Google"},
        {"google.com.ec", "Google"},
        {"google.com.eg", "Google"},
        {"google.com.et", "Google"},
        {"google.com.fj", "Google"},
        {"google.com.gh", "Google"},
        {"google.com.gi", "Google"},
        {"google.com.gt", "Google"},
        {"googletagservices.com", "Google"},
        {"youtube.com", "Google"},
        // Amazon
        {"alexa.com", "Amazon"},
        {"alexametrics.com", "Amazon"},
        {"amazon-adsystem.com", "Amazon"},
        {"amazon.ca", "Amazon"},
        {"amazon.co.jp", "Amazon"},
        {"amazon.co.uk", "Amazon"},
        {"amazon.com", "Amazon"},
        {"amazon.de", "Amazon"},
        {"amazon.es", "Amazon"},
        {"amazon.fr", "Amazon"},
        {"amazon.it", "Amazon"},
        {"amazonaws.com", "Amazon"},
        {"assoc-amazon.com", "Amazon"},
        {"cloudfront.net", "Amazon"},
        {"ssl-images-amazon.com", "Amazon"},
        // Facebook
        {"apps.fbsbx.com", "Facebook"},
        {"atdmt.com", "Facebook"},
        {"atlassolutions.com", "Facebook"},
        {"facebook.com", "Facebook"},
        {"facebook.de", "Facebook"},
        {"facebook.fr", "Facebook"},
        {"facebook.net", "Facebook"},
        {"fb.com", "Facebook"},
        {"fb.me", "Facebook"},
        {"fbcdn.net", "Facebook"},
        {"fbsbx.com", "Facebook"},
        {"friendfeed.com", "Facebook"},
        {"instagram.com", "Facebook"},
        {"messenger.com", "Facebook"},
    });

std::string GetCompanyNameFromGURL(const GURL url) {
  auto host = net::registry_controlled_domains::GetDomainAndRegistry(
      url.host(), net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);

  if (kDomains.contains(host)) {
    base::StringPiece domain = kDomains.at(host);
    return std::string(domain.data(), domain.size());
  }

  return std::string();
}

std::pair<std::string, int> GetCompanyNamesAndCountFromAdsList(
    std::vector<GURL> ads_list) {
  std::string result;
  std::unordered_map<std::string, int> company_count_map;
  int total_count = 0;

  for (GURL url : ads_list) {
    std::string company_name = GetCompanyNameFromGURL(url);
    if (company_name.empty()) {
      continue;
    }
    company_count_map[company_name]++;
  }

  for (const auto& [key, value] : company_count_map) {
    if (value > 0) {
      result.append(key);
      result.append(", ", 2);
      total_count += value;
    }
  }

  if (result.size() > 0) {
    result = result.substr(0, result.size() - 2);
  }

  return {result, total_count};
}
}  // namespace domain_map

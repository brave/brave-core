/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/common/content_settings_util.h"

#include "base/strings/strcat.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace content_settings {

// To control cookies block mode on given |url| in brave shields we need:
//
//   |host_pattern| is *://url.host()/*
//   |domain_pattern| is *://[*.]host_pattern.host()/*
//
// 1. To allow all cookies:
//    Add cookies rule: [*, host_pattern] -> allow
//    * It allows all cookies from any sites on the given host
//
// 2. To block all cookies:
//    Add cookies rule: [*, host_pattern] -> block
//    * It blocks all cookies from any sites on the given host
//
// 3. To block 3p cookies:
//    Add two cookies rules:
//      a. [*, host_pattern] -> block
//      * It blocks all cookies from any sites on the given host
//
//      b. [domain_pattern, host_pattern] -> allow
//      * It allows all cookies from the same site on the given host
//
// Example:
//    For https://www.cnn.com we got:
//       |host_pattern| ==  *://www.cnn.com/*
//       |domain_pattern| == *://[*.]cnn.com/*

ShieldsCookiesPatterns CreateShieldsCookiesPatterns(const GURL& url) {
  ShieldsCookiesPatterns result;
  result.host_pattern = CreateHostPattern(url);
  if (result.host_pattern.GetHost().empty()) {
    result.domain_pattern = result.host_pattern;
    return result;
  }
  result.domain_pattern = CreateDomainPattern(url);
  return result;
}

ContentSettingsPattern CreateHostPattern(const GURL& url) {
  DCHECK(url.is_empty() ? url.possibly_invalid_spec() == "" : url.is_valid());
  if (url.is_empty() && url.possibly_invalid_spec() == "") {
    return ContentSettingsPattern::Wildcard();
  }

  return ContentSettingsPattern::FromString(
      base::StrCat({"*://", url.host(), "/*"}));
}

ContentSettingsPattern CreateDomainPattern(const GURL& url) {
  DCHECK(url.is_empty() ? url.possibly_invalid_spec() == "" : url.is_valid());
  if (url.is_empty() && url.possibly_invalid_spec() == "") {
    return ContentSettingsPattern::Wildcard();
  }

  auto domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (domain.empty()) {
    // IP Address.
    return CreateHostPattern(url);
  }

  return ContentSettingsPattern::FromString(
      base::StrCat({"*://[*.]", domain, "/*"}));
}

}  // namespace content_settings

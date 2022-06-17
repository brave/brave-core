/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/common/content_settings_util.h"

#include "base/strings/strcat.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace content_settings {

// Since Shields work at the domain level, we create a pattern for
// subdomains. In the case of an IP address, we take it as is.
ContentSettingsPattern CreatePrimaryPattern(
    const ContentSettingsPattern& host_pattern) {
  DCHECK(!host_pattern.GetHost().empty());

  auto host = net::registry_controlled_domains::GetDomainAndRegistry(
      host_pattern.GetHost(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (host.empty()) {
    // IP Address.
    return host_pattern;
  }

  return ContentSettingsPattern::FromString(
      base::StrCat({"*://[*.]", host, "/*"}));
}

}  // namespace content_settings

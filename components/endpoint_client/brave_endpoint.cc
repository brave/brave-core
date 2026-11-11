/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/brave_endpoint.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "url/url_constants.h"

namespace endpoint_client::internal {

GURL MakeBraveServiceURL(std::string_view prefix, std::string_view path) {
  return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                            brave_domains::GetServicesDomain(
                                std::string(prefix), brave_domains::STAGING)}))
      .Resolve(path);
}

}  // namespace endpoint_client::internal

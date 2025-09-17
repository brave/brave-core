/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/host.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_account::endpoints {

constexpr char kHostnamePart[] = "accounts.bsg";

const GURL& Host() {
  static const base::NoDestructor<GURL> kHost(
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                    brave_domains::GetServicesDomain(kHostnamePart)}));
  return *kHost;
}

}  // namespace brave_account::endpoints

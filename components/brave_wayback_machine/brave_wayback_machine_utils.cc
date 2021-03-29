/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

bool IsWaybackMachineDisabledFor(const GURL& url) {
  if (net::IsLocalhost(url))
    return true;

  if (base::EndsWith(url.host(), ".local", base::CompareCase::SENSITIVE))
    return true;

  if (base::EndsWith(url.host(), ".onion", base::CompareCase::SENSITIVE))
    return true;

  // Disable on web.archive.org
  if (url.host() == kWaybackHost)
    return true;

  return false;
}

GURL FixupWaybackQueryURL(const GURL& url) {
  // Get latest page always from wayback machine by invalidating timestamp
  // and callback parameters in query string.
  GURL fixed_url = url;
  std::string unused;
  if (net::GetValueForKeyInQuery(fixed_url, "timestamp", &unused)) {
    fixed_url = net::AppendOrReplaceQueryParameter(fixed_url, "timestamp", "");
  }

  if (net::GetValueForKeyInQuery(fixed_url, "callback", &unused)) {
    fixed_url = net::AppendOrReplaceQueryParameter(fixed_url, "callback", "");
  }

  return fixed_url;
}

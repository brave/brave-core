/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/logging.h"

#include "bat/ads/ads_client.h"

namespace ads {

AdsClient* g_ads_client = nullptr;  // NOT OWNED

void set_ads_client_for_logging(
    AdsClient* ads_client) {
  DCHECK(ads_client);
  g_ads_client = ads_client;
}

void Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  if (!g_ads_client) {
    return;
  }

  g_ads_client->Log(file, line, verbose_level, message);
}

}  // namespace ads

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_client_helper.h"

#include "base/check_op.h"

namespace ads {

namespace {
AdsClient* g_ads_client = nullptr;
}  // namespace

AdsClientHelper::AdsClientHelper(AdsClient* ads_client) {
  DCHECK(ads_client);
  DCHECK_EQ(g_ads_client, nullptr);
  g_ads_client = ads_client;
}

AdsClientHelper::~AdsClientHelper() {
  DCHECK(g_ads_client);
  g_ads_client = nullptr;
}

// static
AdsClient* AdsClientHelper::Get() {
  DCHECK(g_ads_client);
  return g_ads_client;
}

// static
bool AdsClientHelper::HasInstance() {
  return g_ads_client;
}

}  // namespace ads

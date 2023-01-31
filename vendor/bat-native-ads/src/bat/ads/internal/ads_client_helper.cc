/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_client_helper.h"

#include "base/check.h"

namespace ads {

namespace {
AdsClient* g_ads_client_instance = nullptr;
}  // namespace

AdsClientHelper::AdsClientHelper(AdsClient* ads_client) {
  DCHECK(ads_client);
  DCHECK(!g_ads_client_instance);
  g_ads_client_instance = ads_client;
}

AdsClientHelper::~AdsClientHelper() {
  DCHECK(g_ads_client_instance);
  g_ads_client_instance = nullptr;
}

// static
AdsClient* AdsClientHelper::GetInstance() {
  DCHECK(g_ads_client_instance);
  return g_ads_client_instance;
}

// static
bool AdsClientHelper::HasInstance() {
  return !!g_ads_client_instance;
}

}  // namespace ads

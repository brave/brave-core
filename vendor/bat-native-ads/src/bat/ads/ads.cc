/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include "bat/ads/internal/ads_impl.h"

namespace ads {

mojom::Environment g_environment = mojom::Environment::kDevelopment;
mojom::SysInfo g_sys_info;
mojom::BuildChannel g_build_channel;

bool g_is_debug = false;

const char g_catalog_schema_resource_id[] = "catalog-schema.json";

// static
Ads* Ads::CreateInstance(AdsClient* ads_client) {
  DCHECK(ads_client);
  return new AdsImpl(ads_client);
}

}  // namespace ads

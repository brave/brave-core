/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_SERVICE_SANDBOX_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_SERVICE_SANDBOX_TYPE_H_

#include "content/public/browser/service_process_host.h"

// bat_ads::mojom::BatAdsService
namespace bat_ads {
namespace mojom {
class BatAdsService;
}  // namespace mojom
}  // namespace bat_ads

template <>
inline sandbox::policy::SandboxType
content::GetServiceSandboxType<bat_ads::mojom::BatAdsService>() {
  return sandbox::policy::SandboxType::kUtility;
}

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_SERVICE_SANDBOX_TYPE_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_content_packaged_service_manifests.h"

#include "base/no_destructor.h"
#include "brave/components/services/bat_ads/public/cpp/manifest.h"
#include "brave/components/services/bat_ledger/public/cpp/manifest.h"
#include "brave/utility/tor/public/cpp/manifest.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

const std::vector<service_manager::Manifest>&
GetBraveContentPackagedServiceManifests() {
  static base::NoDestructor<std::vector<service_manager::Manifest>> manifests{{
      bat_ads::GetManifest(),
      bat_ledger::GetManifest(),
      tor::GetTorLauncherManifest(),
  }};
  return *manifests;
}

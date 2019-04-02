/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_content_packaged_service_overlay_manifest.h"

#include "base/no_destructor.h"
#include "brave/components/services/bat_ads/public/cpp/manifest.h"
#include "brave/components/services/bat_ledger/public/cpp/manifest.h"
#include "brave/utility/tor/public/cpp/manifest.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

const service_manager::Manifest&
GetBraveContentPackagedServiceOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifests{
      service_manager::ManifestBuilder()
          .WithServiceName("content_packaged_services")
          .WithDisplayName("Brave Packaged Services")
          .PackageService(bat_ads::GetManifest())
          .PackageService(bat_ledger::GetManifest())
          .PackageService(tor::GetTorLauncherManifest())
          .Build()};
  return *manifests;
}

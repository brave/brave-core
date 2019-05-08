/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_content_browser_overlay_manifest.h"

#include "base/no_destructor.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/services/bat_ledger/public/cpp/manifest.h"
#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/public/cpp/manifest.h"
#endif
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/utility/tor/public/cpp/manifest.h"
#endif

const service_manager::Manifest& GetBraveContentBrowserOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName("content_browser")
          .WithDisplayName("Brave")
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
          .PackageService(bat_ledger::GetManifest())
#if BUILDFLAG(BRAVE_ADS_ENABLED)
          .PackageService(bat_ads::GetManifest())
#endif
#endif
#if BUILDFLAG(ENABLE_TOR)
          .PackageService(tor::GetTorLauncherManifest())
#endif
          .Build()};
  return *manifest;
}

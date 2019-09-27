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
#endif

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/public/cpp/manifest.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/services/tor/public/cpp/manifest.h"
#endif

const service_manager::Manifest& GetBraveContentBrowserOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName("content_browser")
          .WithDisplayName("Brave")
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
          .RequireCapability("bat_ledger", "bat_ledger")
#if BUILDFLAG(BRAVE_ADS_ENABLED)
          .RequireCapability("bat_ads", "bat_ads")
#endif
#endif
#if BUILDFLAG(ENABLE_TOR)
          .RequireCapability("tor_launcher", "tor_launcher")
#endif
          .Build()};
  return *manifest;
}

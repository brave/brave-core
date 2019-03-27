/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/tor/public/cpp/manifest.h"

#include "base/no_destructor.h"
#include "brave/common/tor/tor_launcher.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

namespace tor {

const service_manager::Manifest& GetTorLauncherManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName(mojom::kTorLauncherServiceName)
          .WithDisplayName("Tor Launcher")
          .WithOptions(service_manager::ManifestOptionsBuilder()
                           .WithSandboxType("none")
                           .Build())
          .ExposeCapability(
              "tor_launcher",
              service_manager::Manifest::InterfaceList<mojom::TorLauncher>())
          .RequireCapability("service_manager", "service_manager:all_users")
          .Build()};
  return *manifest;
}

}  // namespace tor

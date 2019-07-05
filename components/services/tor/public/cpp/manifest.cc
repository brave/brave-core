/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/tor/public/cpp/manifest.h"

#include "base/no_destructor.h"
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

namespace tor {

const service_manager::Manifest& GetTorLauncherManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName(mojom::kServiceName)
          .WithDisplayName("Tor Launcher")
          .WithOptions(service_manager::ManifestOptionsBuilder()
                  .WithExecutionMode(service_manager::Manifest::ExecutionMode::
                                         kOutOfProcessBuiltin)
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

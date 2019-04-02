/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/cpp/manifest.h"

#include "base/no_destructor.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

namespace bat_ledger {

const service_manager::Manifest& GetManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName(mojom::kServiceName)
          .WithDisplayName("Bat Ledger Service")
          .WithOptions(service_manager::ManifestOptionsBuilder()
                           .WithInstanceSharingPolicy(
                               service_manager::Manifest::
                                   InstanceSharingPolicy::kSharedAcrossGroups)
                           .WithSandboxType("none")
                           .Build())
          .ExposeCapability(
              "bat_ledger",
                            service_manager::Manifest::InterfaceList<
                                mojom::BatLedgerService>())
          .Build()};
  return *manifest;
}

}  // namespace bat_ledger

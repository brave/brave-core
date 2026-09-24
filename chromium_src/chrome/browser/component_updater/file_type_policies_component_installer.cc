/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

namespace component_updater {

constexpr char kFileTypePoliciesComponentId[] =
    "khaoiebndkojlmppeemjhbpbandiljpe";

void OnFileTypePoliciesRegistered() {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()->EnsureInstalled(
      kFileTypePoliciesComponentId);
}

}  // namespace component_updater

#include <chrome/browser/component_updater/file_type_policies_component_installer.cc>

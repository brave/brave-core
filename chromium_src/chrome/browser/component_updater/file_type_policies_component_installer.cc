/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define RegisterFileTypePoliciesComponent \
  RegisterFileTypePoliciesComponent_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/file_type_policies_component_installer.cc"  // NOLINT
#undef RegisterFileTypePoliciesComponent

#include "chrome/browser/component_updater/component_updater_utils.h"
#include "components/component_updater/component_updater_service.h"

namespace component_updater {

const char kFileTypePoliciesComponentId[] = "khaoiebndkojlmppeemjhbpbandiljpe";

void OnFileTypePoliciesRegistered() {
  component_updater::BraveOnDemandUpdate(kFileTypePoliciesComponentId);
}

void RegisterFileTypePoliciesComponent(ComponentUpdateService* cus) {
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<FileTypePoliciesComponentInstallerPolicy>());
  installer->Register(cus, base::BindOnce(&OnFileTypePoliciesRegistered));
}

}  // namespace component_updater

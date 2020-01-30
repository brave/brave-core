/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define RegisterCrowdDenyComponent RegisterCrowdDenyComponent_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/crowd_deny_component_installer.cc"  // NOLINT
#undef RegisterCrowdDenyComponent

namespace component_updater {

void RegisterCrowdDenyComponent(ComponentUpdateService* cus,
                                const base::FilePath& user_data_dir) {
  return;
}

}  // namespace component_updater

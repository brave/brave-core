/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_COMPONENT_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_COMPONENT_INSTALLER_H_

#define Register                                     \
  Register_ChromiumImpl(ComponentUpdateService* cus, \
                        base::OnceClosure callback); \
  void Register
#include "../../../../components/component_updater/component_installer.h"
#undef Register

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_COMPONENT_INSTALLER_H_

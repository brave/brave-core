/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_COMPONENT_UPDATER_SERVICE_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_COMPONENT_UPDATER_SERVICE_INTERNAL_H_

#include "components/component_updater/component_updater_service.h"

#define OnDemandUpdate                                                   \
  OnDemandUpdate(const std::vector<std::string>& ids, Priority priority, \
                 Callback callback) override;                            \
  void OnDemandUpdate

#include "src/components/component_updater/component_updater_service_internal.h"  // IWYU pragma: export

#undef OnDemandUpdate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_COMPONENT_UPDATER_SERVICE_INTERNAL_H_

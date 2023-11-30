/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_BLUETOOTH_CHOOSER_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_BLUETOOTH_CHOOSER_CONTROLLER_H_

#include <optional>

#include "components/permissions/chooser_controller.h"

#define GetThrobberLabelAndTooltip(...)                   \
  GetThrobberLabelAndTooltip(__VA_ARGS__) const override; \
  std::optional<ChooserControllerType> GetType()

#include "src/components/permissions/bluetooth_chooser_controller.h"  // IWYU pragma: export

#undef GetThrobberLabelAndTooltip

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_BLUETOOTH_CHOOSER_CONTROLLER_H_

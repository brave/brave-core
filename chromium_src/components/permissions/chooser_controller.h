/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_CHOOSER_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_CHOOSER_CONTROLLER_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace permissions {

enum class ChooserControllerType {
  kBluetooth,
};

}  // namespace permissions

#define GetSelectAllCheckboxLabel(...)          \
  GetSelectAllCheckboxLabel(__VA_ARGS__) const; \
  virtual absl::optional<ChooserControllerType> GetType()

#include "src/components/permissions/chooser_controller.h"  // IWYU pragma: export

#undef GetSelectAllCheckboxLabel

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_CHOOSER_CONTROLLER_H_

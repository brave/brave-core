/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/permissions/chooser_controller.h"

#include "src/components/permissions/chooser_controller.cc"

namespace permissions {

absl::optional<ChooserControllerType> ChooserController::GetType() const {
  return absl::nullopt;
}

}  // namespace permissions

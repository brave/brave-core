// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/default_accelerators.h"

#include "brave/browser/ui/commands/accelerator_service.h"
#include "chrome/browser/ui/views/accelerator_table.h"

namespace commands {

Accelerators GetDefaultAccelerators() {
  commands::Accelerators defaults;
  for (const auto& accelerator_info : GetAcceleratorList()) {
    defaults[accelerator_info.command_id].push_back(
        ui::Accelerator(accelerator_info.keycode, accelerator_info.modifiers));
  }
  return defaults;
}

}  // namespace commands

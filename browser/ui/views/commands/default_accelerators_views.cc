// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/default_accelerators.h"

#include "brave/browser/ui/commands/accelerator_service.h"
#include "chrome/browser/ui/views/accelerator_table.h"

#if BUILDFLAG(IS_MAC)
#include "brave/browser/ui/views/commands/default_accelerators_mac.h"
#endif  // BUILDFLAG(IS_MAC)

namespace commands {

Accelerators GetDefaultAccelerators() {
  Accelerators defaults;
  auto add_to_accelerators = [&defaults](const AcceleratorMapping& mapping) {
    defaults[mapping.command_id].push_back(
        ui::Accelerator(mapping.keycode, mapping.modifiers));
  };

  base::ranges::for_each(GetAcceleratorList(), add_to_accelerators);
#if BUILDFLAG(IS_MAC)
  // TODO(sko) These accelerators should be flagged as unmodifiable unless we
  // can modify the OS settings. See the comment in default_accelerator_mac.h
  base::ranges::for_each(GetGlobalAccelerators(), add_to_accelerators);
#endif  // BUILDFLAG(IS_MAC)
  return defaults;
}

}  // namespace commands

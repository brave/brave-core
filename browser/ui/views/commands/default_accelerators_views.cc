// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility>

#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/commands/default_accelerators.h"
#include "chrome/browser/ui/views/accelerator_table.h"
#include "ui/base/accelerators/accelerator.h"

#if BUILDFLAG(IS_MAC)
#include "brave/browser/ui/views/commands/default_accelerators_mac.h"
#endif  // BUILDFLAG(IS_MAC)

namespace commands {

std::pair<Accelerators, base::flat_set<ui::Accelerator>>
GetDefaultAccelerators() {
  std::pair<Accelerators, base::flat_set<ui::Accelerator>> result;
  auto& [defaults, system_commands] = result;

  auto add_to_accelerators = [&defaults](const AcceleratorMapping& mapping) {
    defaults[mapping.command_id].push_back(
        ui::Accelerator(mapping.keycode, mapping.modifiers));
  };

  base::ranges::for_each(GetAcceleratorList(), add_to_accelerators);
#if BUILDFLAG(IS_MAC)
  // TODO(sko) These accelerators should be flagged as system_commands unless we
  // can modify the OS settings. See the comment in default_accelerator_mac.h
  base::ranges::for_each(GetGlobalAccelerators(), add_to_accelerators);
  base::ranges::for_each(GetGlobalAccelerators(),
                         [&system_commands](const AcceleratorMapping& mapping) {
                           system_commands.insert(ui::Accelerator(
                               mapping.keycode, mapping.modifiers));
                         });
#endif  // BUILDFLAG(IS_MAC)
  return result;
}

}  // namespace commands

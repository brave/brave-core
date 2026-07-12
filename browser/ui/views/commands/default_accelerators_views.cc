// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "brave/browser/ui/commands/default_accelerators.h"
#include "build/build_config.h"
#include "chrome/browser/ui/accelerator_table.h"
#include "ui/base/accelerators/accelerator.h"

#if BUILDFLAG(IS_MAC)
#include "brave/browser/ui/views/commands/default_accelerators_mac.h"
#endif  // BUILDFLAG(IS_MAC)

namespace commands {

DefaultAccelerators::DefaultAccelerators() = default;
DefaultAccelerators::~DefaultAccelerators() = default;
DefaultAccelerators::DefaultAccelerators(DefaultAccelerators&&) = default;
DefaultAccelerators& DefaultAccelerators::operator=(DefaultAccelerators&&) =
    default;

DefaultAccelerators GetDefaultAccelerators() {
  DefaultAccelerators result;

  auto add_to_accelerators =
      [&result](const AcceleratorMapping& mapping) {
        result.accelerators[mapping.command_id].push_back(
            ui::Accelerator(mapping.keycode, mapping.modifiers));
      };

  std::ranges::for_each(GetAcceleratorList(), add_to_accelerators);
#if BUILDFLAG(IS_MAC)
  const MacGlobalAccelerators& global_accelerators = GetGlobalAccelerators();
  std::ranges::for_each(global_accelerators.all, add_to_accelerators);
  for (const AcceleratorMapping& mapping : global_accelerators.menu_backed) {
    result.menu_dispatched[mapping.command_id].push_back(
        ui::Accelerator(mapping.keycode, mapping.modifiers));
  }
  for (const AcceleratorMapping& mapping : global_accelerators.unmodifiable) {
    result.system_managed.insert(
        ui::Accelerator(mapping.keycode, mapping.modifiers));
  }
#endif  // BUILDFLAG(IS_MAC)
  return result;
}

}  // namespace commands

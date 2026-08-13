// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "brave/browser/ui/commands/default_accelerators.h"
#include "build/build_config.h"
#include "chrome/browser/ui/accelerator_table.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

DefaultAccelerators::DefaultAccelerators() = default;
DefaultAccelerators::~DefaultAccelerators() = default;
DefaultAccelerators::DefaultAccelerators(DefaultAccelerators&&) = default;
DefaultAccelerators& DefaultAccelerators::operator=(DefaultAccelerators&&) =
    default;

DefaultAccelerators GetDefaultAccelerators() {
  DefaultAccelerators result;

  std::ranges::for_each(
      GetAcceleratorList(), [&result](const AcceleratorMapping& mapping) {
        result.accelerators[mapping.command_id].push_back(
            ui::Accelerator(mapping.keycode, mapping.modifiers));
      });
#if BUILDFLAG(IS_MAC)
  const DefaultAccelerators& global_accelerators = GetGlobalAccelerators();
  for (const auto& [command_id, accelerators] :
       global_accelerators.accelerators) {
    auto& command_accelerators = result.accelerators[command_id];
    command_accelerators.insert(command_accelerators.end(),
                                accelerators.begin(), accelerators.end());
  }
  result.menu_dispatched = global_accelerators.menu_dispatched;
  result.system_managed = global_accelerators.system_managed;
#endif  // BUILDFLAG(IS_MAC)
  return result;
}

}  // namespace commands

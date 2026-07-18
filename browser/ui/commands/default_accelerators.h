// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDS_DEFAULT_ACCELERATORS_H_
#define BRAVE_BROWSER_UI_COMMANDS_DEFAULT_ACCELERATORS_H_

#include "base/containers/flat_set.h"
#include "brave/components/commands/browser/accelerator_pref_manager.h"
#include "build/build_config.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

struct DefaultAccelerators {
  DefaultAccelerators();
  ~DefaultAccelerators();
  DefaultAccelerators(DefaultAccelerators&&);
  DefaultAccelerators& operator=(DefaultAccelerators&&);

  // The default accelerators for each command.
  AcceleratorPrefManager::Accelerators accelerators;

#if BUILDFLAG(IS_MAC)
  // Accelerators that can't be modified or removed by the user, and are not
  // registered with the browser's FocusManager as the OS dispatches them.
  base::flat_set<ui::Accelerator> system_managed;

  // Accelerators dispatched via a main menu NSMenuItem key equivalent. They
  // can be modified or removed by the user (the menu is kept in sync by
  // AcceleratorMenuCoordinatorMac), but must not be registered with the
  // FocusManager while assigned to their default command, to avoid double
  // handling.
  AcceleratorPrefManager::Accelerators menu_dispatched;
#endif  // BUILDFLAG(IS_MAC)
};

// Gets the default list of accelerators.
DefaultAccelerators GetDefaultAccelerators();

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_COMMANDS_DEFAULT_ACCELERATORS_H_

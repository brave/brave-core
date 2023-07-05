// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDS_DEFAULT_ACCELERATORS_H_
#define BRAVE_BROWSER_UI_COMMANDS_DEFAULT_ACCELERATORS_H_

#include <utility>

#include "base/containers/flat_set.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

// Gets the default list of accelerators.
std::pair<Accelerators, base::flat_set<ui::Accelerator>>
GetDefaultAccelerators();

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_COMMANDS_DEFAULT_ACCELERATORS_H_

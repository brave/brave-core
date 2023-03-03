// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDS_COMMON_KEY_NAMES_H_
#define BRAVE_COMPONENTS_COMMANDS_COMMON_KEY_NAMES_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace commands {

// Gets a string representing a keyboard key.
COMPONENT_EXPORT(COMMANDS_COMMON) std::string GetKeyName(ui::KeyboardCode code);

// Gets a string representing an accelerator modifier.
COMPONENT_EXPORT(COMMANDS_COMMON)
std::vector<std::string> GetModifierName(ui::KeyEventFlags flags);

}  // namespace commands

#endif  // BRAVE_COMPONENTS_COMMANDS_COMMON_KEY_NAMES_H_

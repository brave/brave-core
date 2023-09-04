// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDS_COMMON_ACCELERATOR_PARSING_H_
#define BRAVE_COMPONENTS_COMMANDS_COMMON_ACCELERATOR_PARSING_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

// Converts a DomCode string to a human readable key string.
COMPONENT_EXPORT(COMMANDS_COMMON)
std::string CodeStringToKeyString(const std::string& code_string);

// Converts an accelerator to a DomKeysString, which is all the DomKeys joined
// around a '+' character.
// Note: a KeysString is only really useful for displaying to  the user, as it
// depends on the Keyboard Layout. Currently this defaults to the US Layout, but
// that won't always be the case.
COMPONENT_EXPORT(COMMANDS_COMMON)
std::string ToKeysString(const ui::Accelerator& accelerator);

// Converts an accelerator to a DomCodesString, which is all the DomCodes joined
// around a '+' character. Note: Modifiers are converted to an unlocated version
// (i.e. ControlLeft ==> Control).
COMPONENT_EXPORT(COMMANDS_COMMON)
std::string ToCodesString(const ui::Accelerator& accelerator);

// Parses a CodesString into an accelerator. For example Control+Alt+KeyG would
// be parsed into an accelerator with the Control & Alt modifiers, and
// VKEY_G as the key_code.
COMPONENT_EXPORT(COMMANDS_COMMON)
ui::Accelerator FromCodesString(const std::string& value);

}  // namespace commands

#endif  // BRAVE_COMPONENTS_COMMANDS_COMMON_ACCELERATOR_PARSING_H_

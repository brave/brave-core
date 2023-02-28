// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_PARSING_H_
#define BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_PARSING_H_

#include <string>
#include <vector>

#include "ui/base/accelerators/accelerator.h"

namespace commands {

// Converts an accelerator to a DomKeysString, which is all the DomKeys joined
// around a '+' character.
// Note: a KeysString is only really useful for displaying to  the user, as it
// depends on the Keyboard Layout. Currently this defaults to the US Layout, but
// that won't always be the case.
std::string ToKeysString(const ui::Accelerator& accelerator);

// Converts an accelerator to a DomCodesString, which is all the DomCodes joined
// around a '+' character. Note: Modifiers are converted to an unlocated version
// (i.e. ControlLeft ==> Control).
std::string ToCodesString(const ui::Accelerator& accelerator);

// Parses a CodesString into an accelerator. For example Control+Alt+KeyG would
// be parsed into an accelerator with the Control & Alt modifiers, and
// VKEY_G as the key_code.
ui::Accelerator FromCodesString(const std::string& value);

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_PARSING_H_

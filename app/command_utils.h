// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_APP_COMMAND_UTILS_H_
#define BRAVE_APP_COMMAND_UTILS_H_

#include <string>

#include "base/containers/span.h"

namespace commands {

// Gets the command ids of all commands which don't require parameters and can
// be executed in the main browser window. This is used for listing the
// shortcuts available to users and will eventually be used to allow configuring
// shortcuts.
base::span<const int> GetCommands();

// Gets a string representing a command. In future this will be translated, but
// while we're prototyping the feature it will always returns English strings.
std::string GetCommandName(int command_id);

}  // namespace commands

#endif  // BRAVE_APP_COMMAND_UTILS_H_

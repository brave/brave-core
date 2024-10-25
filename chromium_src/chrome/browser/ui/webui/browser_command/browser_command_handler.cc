/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/notreached.h"

// The switch statement in `BrowserCommandHandler::CanExecuteCommand` does not
// include a default clause. As we are adding values to the enumeration being
// handled, we must add a default clause in order to avoid a compile error.
#define BRAVE_CAN_EXECUTE_COMMAND \
  default:                        \
    NOTREACHED_NORETURN();

#include "src/chrome/browser/ui/webui/browser_command/browser_command_handler.cc"

#undef BRAVE_CAN_EXECUTE_COMMAND

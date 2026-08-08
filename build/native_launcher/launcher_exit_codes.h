/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_EXIT_CODES_H_
#define BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_EXIT_CODES_H_

// Standardized exit codes for the launcher infrastructure.
enum LauncherExitCode : int {
  // sysexits.h: EX_CONFIG (Configuration error)
  kExitInvalidConfig = 78,

  // POSIX standard: Command cannot execute
  kExitCannotExecute = 126,
};

#endif  // BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_EXIT_CODES_H_

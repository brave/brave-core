/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_DIALOG_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_DIALOG_H_

#include "build/build_config.h"

#include "src/chrome/browser/first_run/first_run_dialog.h"

#if BUILDFLAG(IS_WIN)

class Profile;

namespace first_run {

// Enable first run dialog on Win also.
// Upstream only uses it for macOS/Linux.
void ShowFirstRunDialog(Profile* profile);

}  // namespace first_run

#endif  // BUILDFLAG(IS_WIN)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_DIALOG_H_

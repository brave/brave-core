/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_WIN_H_
#define BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_WIN_H_

class Profile;

namespace shell_integration::win {

// Pin profile-specific shortcut when |profile| is non-null.
void PinToTaskbar(Profile* profile = nullptr);

}  // namespace shell_integration::win

#endif  // BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_WIN_H_

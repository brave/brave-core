/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_MAC_H_
#define BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_MAC_H_

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"

namespace shell_integration::mac {

// No-op if already added.
void AddIconToDock(
    base::OnceCallback<void(bool)> result_callback = base::DoNothing());
void IsIconAddedToDock(base::OnceCallback<void(bool)> result_callback);
}  // namespace shell_integration::mac

#endif  // BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_MAC_H_

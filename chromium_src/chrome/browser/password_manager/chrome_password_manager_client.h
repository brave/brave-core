/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PASSWORD_MANAGER_CHROME_PASSWORD_MANAGER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PASSWORD_MANAGER_CHROME_PASSWORD_MANAGER_CLIENT_H_

// Include the base class header first (unmangled) so the macros below only
// split the derived declarations in ChromePasswordManagerClient, not the
// identically-named virtual methods on PasswordManagerClient.
#include "components/password_manager/core/browser/password_manager_client.h"

// Split each of these two virtual declarations into a non-virtual
// _ChromiumImpl (the upstream body) plus the original virtual override. Brave's
// overrides (defined in the accompanying .cc) gate password filling and saving
// on the kBravePasswordManagerFillEnabled pref before delegating to the impls.
// Both methods are entered by external translation units through the virtual
// interface (IsFillingEnabled from PasswordManager::CreateFormManager,
// IsSavingAndFillingEnabled from PasswordFormManager), so the gate is applied
// on those user-facing paths.
#define IsSavingAndFillingEnabled                                \
  IsSavingAndFillingEnabled_ChromiumImpl(const GURL& url) const; \
  bool IsSavingAndFillingEnabled
#define IsFillingEnabled                                \
  IsFillingEnabled_ChromiumImpl(const GURL& url) const; \
  bool IsFillingEnabled

#include <chrome/browser/password_manager/chrome_password_manager_client.h>  // IWYU pragma: export

#undef IsFillingEnabled
#undef IsSavingAndFillingEnabled

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PASSWORD_MANAGER_CHROME_PASSWORD_MANAGER_CLIENT_H_

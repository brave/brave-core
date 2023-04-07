/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_IDENTITY_IDENTITY_GET_AUTH_TOKEN_FUNCTION_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_IDENTITY_IDENTITY_GET_AUTH_TOKEN_FUNCTION_H_

#include "brave/browser/extensions/api/identity/brave_web_auth_flow.h"
#include "extensions/browser/extension_function.h"

#define ExtensionFunction ExtensionFunction, public BraveWebAuthFlow
#include "src/chrome/browser/extensions/api/identity/identity_get_auth_token_function.h"  // IWYU pragma: export
#undef ExtensionFunction

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_IDENTITY_IDENTITY_GET_AUTH_TOKEN_FUNCTION_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_ETHEREUM_REMOTE_CLIENT_UTIL_H_
#define BRAVE_BROWSER_EXTENSIONS_ETHEREUM_REMOTE_CLIENT_UTIL_H_

#include <string>

class PrefService;

namespace extensions {

bool ShouldLoadEthereumRemoteClientExtension(PrefService* prefs);

std::string GetInfuraProjectID();

std::string GetBraveKey();

bool HasInfuraProjectID();

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_ETHEREUM_REMOTE_CLIENT_UTIL_H_

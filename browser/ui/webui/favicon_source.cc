// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/constants/webui_url_constants.h"

// Brave's chrome-untrusted:// WebUIs that load chrome-untrusted://favicon2,
// which is cross-origin to them.
bool IsBraveUntrustedFaviconOrigin(const std::string& origin) {
  return base::StrCat({origin, "/"}) == kAIChatUntrustedConversationUIURL;
}

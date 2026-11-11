// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/constants/webui_url_constants.h"

// Whether `origin` may load from a FaviconSource. Only the
// chrome-untrusted://favicon2 variant (`serve_untrusted`) needs this, as it is
// cross-origin to every chrome-untrusted:// WebUI that uses it.
bool BraveIsFavicon2AllowedOnOrigin(bool serve_untrusted,
                                    const std::string& origin) {
  return serve_untrusted &&
         base::StrCat({origin, "/"}) == kAIChatUntrustedConversationUIURL;
}

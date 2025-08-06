/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_web_ui_view_utils.h"

#include "brave/components/constants/webui_url_constants.h"
#include "url/gurl.h"

namespace brave {

bool ShouldEnableContextMenu(const GURL& url) {
  return url.is_valid() && url.host() == kAIChatUIHost;
}

}  // namespace brave

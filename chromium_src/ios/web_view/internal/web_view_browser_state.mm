// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/web_view/internal/web_view_browser_state.mm"

namespace ios_web_view {

PrefService* WebViewBrowserState::GetPrefs_Unused() {
  return nullptr;
}

}  // namespace ios_web_view

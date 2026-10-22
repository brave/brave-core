// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/content/navigation/content_navigation_manager.mm>

namespace web {

// TODO: wire to content session restore if Blink iOS moves past the PoC.
bool ContentNavigationManager::IsNativeRestoreInProgress() const {
  return false;
}

}  // namespace web

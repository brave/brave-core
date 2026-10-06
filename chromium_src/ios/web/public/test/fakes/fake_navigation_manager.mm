// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/public/test/fakes/fake_navigation_manager.mm>

namespace web {

bool FakeNavigationManager::IsNativeRestoreInProgress() const {
  return false;
}

}  // namespace web

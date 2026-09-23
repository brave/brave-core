// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/navigation/navigation_manager_impl.mm>

// Implements the new method added to the public NavigationManager override
namespace web {

bool NavigationManagerImpl::IsNativeRestoreInProgress() const {
  return native_restore_in_progress_;
}

}  // namespace web

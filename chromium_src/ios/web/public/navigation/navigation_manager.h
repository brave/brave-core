// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_NAVIGATION_NAVIGATION_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_NAVIGATION_NAVIGATION_MANAGER_H_

// This override simply adds a new public method that will expose if a session
// restore is in progress which can be useful for certain tab helpers
#define CanGoBack                        \
  IsNativeRestoreInProgress() const = 0; \
  virtual bool CanGoBack

#include <ios/web/public/navigation/navigation_manager.h>  // IWYU pragma: export

#undef CanGoBack

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_NAVIGATION_NAVIGATION_MANAGER_H_

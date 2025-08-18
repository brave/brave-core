// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Unlike Chromium we also load the ThemeSource in the system profile, where the
// ThemeService is |nullptr|. We add an empty `if (!theme_service)` and put the
// rest of the if block from upstream in the else block so it doesn't get
// executed when the ThemeService doesn't exist.
#define BRAVE_THEME_SOURCE_CHECK_THEME_SERVICE_EXISTS \
  if (!theme_service) {                               \
    CHECK(profile_->IsSystemProfile());               \
  } else  // NOLINT(readability/braces)

#include <chrome/browser/ui/webui/theme_source.cc>

#undef BRAVE_THEME_SOURCE_CHECK_THEME_SERVICE_EXISTS

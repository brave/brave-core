// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/resources/grit/ui_resources.h"

// ui_resources.grd ships the larger favicons only on some platforms.
#if !defined(IDR_DEFAULT_FAVICON_32)
constexpr int BRAVE_IDR_DEFAULT_FAVICON_32 = IDR_DEFAULT_FAVICON;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_64 = IDR_DEFAULT_FAVICON;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_32 = IDR_DEFAULT_FAVICON_DARK;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_64 = IDR_DEFAULT_FAVICON_DARK;
#else
constexpr int BRAVE_IDR_DEFAULT_FAVICON_32 = IDR_DEFAULT_FAVICON_32;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_64 = IDR_DEFAULT_FAVICON_64;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_32 = IDR_DEFAULT_FAVICON_DARK_32;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_64 = IDR_DEFAULT_FAVICON_DARK_64;
#endif  // !defined(IDR_DEFAULT_FAVICON_32)

#include <chrome/browser/extensions/favicon/favicon_util.cc>

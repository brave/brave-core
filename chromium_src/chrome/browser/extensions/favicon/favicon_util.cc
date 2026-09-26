// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/resources/grit/ui_resources.h"

// ui_resources.grd ships the larger favicons only on some platforms.
#if !defined(IDR_DEFAULT_FAVICON_32)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_ALIASED_DEFAULT_FAVICONS
#define IDR_DEFAULT_FAVICON_32 IDR_DEFAULT_FAVICON
#define IDR_DEFAULT_FAVICON_64 IDR_DEFAULT_FAVICON
#define IDR_DEFAULT_FAVICON_DARK_32 IDR_DEFAULT_FAVICON_DARK
#define IDR_DEFAULT_FAVICON_DARK_64 IDR_DEFAULT_FAVICON_DARK
#endif  // !defined(IDR_DEFAULT_FAVICON_32)

#include <chrome/browser/extensions/favicon/favicon_util.cc>

#if defined(BRAVE_ALIASED_DEFAULT_FAVICONS)
#undef IDR_DEFAULT_FAVICON_DARK_64
#undef IDR_DEFAULT_FAVICON_DARK_32
#undef IDR_DEFAULT_FAVICON_64
#undef IDR_DEFAULT_FAVICON_32
#undef BRAVE_ALIASED_DEFAULT_FAVICONS
#endif  // defined(BRAVE_ALIASED_DEFAULT_FAVICONS)

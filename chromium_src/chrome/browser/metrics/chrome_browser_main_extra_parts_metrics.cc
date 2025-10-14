// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/metrics/chrome_browser_main_extra_parts_metrics.h"

#include "chrome/browser/chrome_browser_main.h"

#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include <chrome/browser/metrics/chrome_browser_main_extra_parts_metrics.cc>
#undef ChromeBrowserMainParts

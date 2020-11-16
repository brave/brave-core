/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_

#include "chrome/browser/search/search_provider_observer.h"

// Forward declaration for class defined in .cc override.
class AlwaysGoogleSearchProviderObserver;

#define SearchProviderObserver AlwaysGoogleSearchProviderObserver
#include "../../../../../chrome/browser/search/instant_service.h"
#undef SearchProviderObserver

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_

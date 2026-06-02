// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_JS_MESSAGING_ORIGIN_FILTER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_JS_MESSAGING_ORIGIN_FILTER_H_

// Adds Brave specific origin filters to the OriginFilter enum
#define kGoogleSearch \
  kGoogleSearch, kYouTube, kBraveSearch, kBraveTalk, kBraveAccount

#include <ios/web/public/js_messaging/origin_filter.h>  // IWYU pragma: export

#undef kGoogleSearch

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_JS_MESSAGING_ORIGIN_FILTER_H_

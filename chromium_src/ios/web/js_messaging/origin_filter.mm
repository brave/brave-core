// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/public/js_messaging/origin_filter.h"

// Replace the GetOriginList method with one that will handle additional origin
// filters and insert a `default` switch case into the Chromium implementation
// since it will no longer handle every available enum case.
#define GetOriginList GetOriginList_Chromium
#define kPublic            \
  kPublic: { return nil; } \
  default

#include <ios/web/js_messaging/origin_filter.mm>

#undef kPublic
#undef GetOriginList

namespace web {

NSArray<NSString*>* GetOriginList(web::OriginFilter filter) {
  switch (filter) {
    case web::OriginFilter::kYouTube:
      return @[
        @"https://youtube.com",
        @"https://www.youtube.com",
        @"https://m.youtube.com",
      ];
    case web::OriginFilter::kBraveSearch:
      return @[
        @"https://safesearch.brave.com",
        @"https://safesearch.brave.software",
        @"https://safesearch.bravesoftware.com",
        @"https://search-dev-local.brave.com",
        @"https://search.brave.com",
        @"https://search.brave.software",
        @"https://search.bravesoftware.com",
      ];
    case web::OriginFilter::kBraveTalk:
      return @[
        @"https://talk.brave.com",
        @"https://talk.bravesoftware.com",
        @"https://talk.brave.software",
      ];
    case web::OriginFilter::kBraveAccount:
      return @[
        @"https://account.brave.com",
        @"https://account.bravesoftware.com",
        @"https://account.brave.software",
      ];
    default:
      return web::GetOriginList_Chromium(filter);
  }
}

}  // namespace web

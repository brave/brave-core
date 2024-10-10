/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BITMAP_FETCHER_BITMAP_FETCHER_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BITMAP_FETCHER_BITMAP_FETCHER_SERVICE_H_

#define RequestImage                                               \
  RequestImageWithNetworkTrafficAnnotationTag(                     \
      const GURL& url, BitmapFetchedCallback callback,             \
      const net::NetworkTrafficAnnotationTag& traffic_annotation); \
  RequestId RequestImage

#include "src/chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"  // IWYU pragma: export
#undef RequestImage

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BITMAP_FETCHER_BITMAP_FETCHER_SERVICE_H_

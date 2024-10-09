/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/bitmap_fetcher/bitmap_fetcher_service.cc"

BitmapFetcherService::RequestId
BitmapFetcherService::RequestImageWithNetworkTrafficAnnotationTag(
    const GURL& url,
    BitmapFetchedCallback callback,
    const net::NetworkTrafficAnnotationTag& tag) {
  return RequestImageImpl(url, std::move(callback), tag);
}

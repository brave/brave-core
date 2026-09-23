/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/assist_ranker/ranker_url_fetcher.h"

#include "services/network/public/cpp/simple_url_loader.h"

namespace assist_ranker {

RankerURLFetcher::RankerURLFetcher()
    : state_(IDLE), retry_count_(0), max_retry_on_5xx_(0) {}

RankerURLFetcher::~RankerURLFetcher() = default;

bool RankerURLFetcher::Request(
    const GURL& url,
    RankerURLFetcher::Callback callback,
    network::mojom::URLLoaderFactory* url_loader_factory) {
  // A false return means no request was started and `callback` will never
  // run, which is already part of the documented contract (see the header):
  // the sole caller, RankerModelLoaderImpl::StartLoadFromURL, checks this
  // return value and finalizes its own state without waiting on `callback`.
  return false;
}

}  // namespace assist_ranker

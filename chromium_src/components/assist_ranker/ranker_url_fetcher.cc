/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/assist_ranker/ranker_url_fetcher.h"

#define Request Request_ChromiumImpl
#include "src/components/assist_ranker/ranker_url_fetcher.cc"
#undef Request

namespace assist_ranker {

bool RankerURLFetcher::Request(
    const GURL& url,
    RankerURLFetcher::Callback callback,
    network::mojom::URLLoaderFactory* url_loader_factory) {
  return false;
}

}  // namespace assist_ranker

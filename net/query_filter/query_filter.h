/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_QUERY_FILTER_QUERY_FILTER_H_
#define BRAVE_NET_QUERY_FILTER_QUERY_FILTER_H_

#include "net/base/net_export.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace net::query_filter {

// Return a new GURL with all trackers removed from from the URL's
// query string if any known trackers were found.
NET_EXPORT absl::optional<GURL> ApplyQueryFilter(const GURL& original_url);

}  // namespace net::query_filter

#endif  // BRAVE_NET_QUERY_FILTER_QUERY_FILTER_H_

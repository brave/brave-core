/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_QUERY_FILTER_H_
#define BRAVE_BROWSER_NET_BRAVE_QUERY_FILTER_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

absl::optional<GURL> ApplyQueryFilter(const GURL& original_url);

#endif  // BRAVE_BROWSER_NET_BRAVE_QUERY_FILTER_H_

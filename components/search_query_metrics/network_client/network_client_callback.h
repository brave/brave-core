/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_NETWORK_CLIENT_NETWORK_CLIENT_CALLBACK_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_NETWORK_CLIENT_NETWORK_CLIENT_CALLBACK_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"

class GURL;

namespace metrics {

using SendRequestCallback = base::OnceCallback<void(
    const GURL& url,
    int response_code,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers)>;

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_NETWORK_CLIENT_NETWORK_CLIENT_CALLBACK_H_

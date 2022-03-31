// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_NETWORK_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_NETWORK_H_

#include "net/traffic_annotation/network_traffic_annotation.h"

namespace brave_news {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag();

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_NETWORK_H_

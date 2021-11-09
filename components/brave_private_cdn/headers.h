// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_HEADERS_H_
#define BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_HEADERS_H_

#include <string>

#include "base/containers/flat_map.h"

namespace brave {

const base::flat_map<std::string, std::string> private_cdn_headers = {
    {"User-Agent", ""},
    {"Accept-Language", ""}};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_HEADERS_H_

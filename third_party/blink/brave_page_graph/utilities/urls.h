/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

std::string URLToString(const blink::KURL& url);
blink::KURL NormalizeUrl(const blink::KURL& url);

}  // namespace brave_page_graph

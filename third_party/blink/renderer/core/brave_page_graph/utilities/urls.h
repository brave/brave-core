/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_UTILITIES_URLS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_UTILITIES_URLS_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace brave_page_graph {

blink::KURL NormalizeUrl(const blink::KURL& url);

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_UTILITIES_URLS_H_

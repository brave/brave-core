/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_REQUEST_METADATA_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_REQUEST_METADATA_H_

#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class RequestMetadata final {
public:
  RequestMetadata(const WTF::String& response_header,
      const int64_t response_body_length)
      : response_header_(response_header),
        response_body_length_(response_body_length) {}

  RequestMetadata()
      : response_header_(WTF::g_empty_string),
        response_body_length_(0) {}

  const WTF::String& response_header_;
  const int64_t response_body_length_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_REQUEST_METADATA_H_

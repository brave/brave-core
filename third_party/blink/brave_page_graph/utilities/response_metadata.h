/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_RESPONSE_METADATA_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_RESPONSE_METADATA_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace blink {
class ResourceResponse;
}

namespace brave_page_graph {

class ResponseMetadata final {
 public:
  ResponseMetadata();
  ResponseMetadata(const blink::ResourceResponse& response);

  const std::string& GetResponseHeaderSummary() const;
  int64_t GetResponseBodyLength() const;
  bool GetIsCookieSetting() const;

 protected:
  std::string response_header_summary_;
  int64_t response_body_length_;
  bool is_cookie_setting_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_RESPONSE_METADATA_H_

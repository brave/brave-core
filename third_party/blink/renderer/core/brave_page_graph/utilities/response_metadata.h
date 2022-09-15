/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_UTILITIES_RESPONSE_METADATA_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_UTILITIES_RESPONSE_METADATA_H_

#include <string>
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace blink {
class ResourceResponse;
}

namespace brave_page_graph {

class ResponseMetadata final {
 public:
  ResponseMetadata();

  void ProcessResourceResponse(const blink::ResourceResponse& response);

  int64_t EncodedDataLength() const { return encoded_data_length_; }
  void SetEncodedDataLength(int64_t encoded_data_length) {
    encoded_data_length_ = encoded_data_length;
  }

  const std::string& GetResponseHeaderSummary() const {
    return response_header_summary_;
  }

 protected:
  std::string response_header_summary_;
  int64_t encoded_data_length_ = -1;
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_UTILITIES_RESPONSE_METADATA_H_

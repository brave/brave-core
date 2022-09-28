/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"

#include <sstream>

#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"

using ::blink::HTTPHeaderMap;
using ::blink::ResourceResponse;

namespace brave_page_graph {

namespace {

void SerializeHeaderMap(const HTTPHeaderMap& headers,
                        const char* prefix,
                        std::ostream& ss) {
  for (const auto& header : headers) {
    ss << prefix << ":" << header.key << " " << header.value << "\n";
  }
}

}  // namespace

ResponseMetadata::ResponseMetadata() = default;

void ResponseMetadata::ProcessResourceResponse(
    const blink::ResourceResponse& response) {
  std::stringstream ss;
  SerializeHeaderMap(response.HttpHeaderFields(), "cooked-response", ss);
  response_header_summary_ = ss.str();
}

}  // namespace brave_page_graph

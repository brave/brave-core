/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/utilities/response_metadata.h"
#include <sstream>
#include <string>
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"

using ::blink::HTTPHeaderMap;
using ::blink::ResourceResponse;
using ::std::stringstream;
using ::std::string;

namespace brave_page_graph {

ResponseMetadata::ResponseMetadata() {}

ResponseMetadata::ResponseMetadata(const ResourceResponse& response) {
  stringstream ss;

  const HTTPHeaderMap& headers = response.HttpHeaderFields();
  for (HTTPHeaderMap::const_iterator it = headers.begin();
      it != headers.end(); ++it) {
    ss << it->key << " " << it->value << "\n";
  }

  response_header_summary_ = ss.str();
  response_body_length_ = response.EncodedBodyLength();
  is_cookie_setting_ = false;
}

const string& ResponseMetadata::GetResponseHeaderSummary() const {
  return response_header_summary_;
}

int64_t ResponseMetadata::GetResponseBodyLength() const {
  return response_body_length_;
}

bool ResponseMetadata::GetIsCookieSetting() const {
  return is_cookie_setting_;
}

}  // namespace brave_page_graph
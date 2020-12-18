/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/private_channel/browser/request_utils.h"

#include <string>
#include "brave/components/private_channel/browser/constants.h"

namespace private_channel {

std::string BuildUrl(const EndpointType endpoint,
                     const std::string api_version) {
  std::string url;

  std::string base_url = PRIVATE_CHANNEL_DEVELOPMENT_SERVER;
  switch (endpoint) {
    case EndpointType::META:
      url = base_url + PRIVATE_CHANNEL_API_VERSION +
            PRIVATE_CHANNEL_META_ENDPOINT;
      break;
    case EndpointType::FIRST_ROUND:
      url = base_url + PRIVATE_CHANNEL_API_VERSION +
            PRIVATE_CHANNEL_FIRST_ROUND_ENDPOINT;
      break;
    case EndpointType::SECOND_ROUND:
      url = base_url + PRIVATE_CHANNEL_API_VERSION +
            PRIVATE_CHANNEL_SECOND_ROUND_ENDPOINT;
      break;
  }
  return url;
}

}  // namespace private_channel

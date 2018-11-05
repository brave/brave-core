/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/callback_handler.h"

namespace ads {

bool CallbackHandler::OnURLSessionReceivedResponse(
      const uint64_t /* session_id */,
      const std::string& /* url */,
      const int /* response_status_code */,
      const std::string& /* response */,
      const std::map<std::string, std::string>& /* headers */) {
  return false;
}

}  // namespace ads

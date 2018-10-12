/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

namespace ads {

enum Result {
  ADS_OK = 0,
  ADS_ERROR = 1,
  NOT_FOUND
};

// CallbackHandler must not be destroyed if they have pending callbacks
class CallbackHandler {
 public:
  virtual ~CallbackHandler() = default;

  virtual void OnURLRequestResponse(uint64_t request_id,
      const std::string& url, int response_code, const std::string& response,
      const std::map<std::string, std::string>& headers) {};
};

}  // namespace ads

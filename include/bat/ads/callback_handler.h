/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

#include "bat/ads/category_info.h"
#include "bat/ads/export.h"

namespace ads {

enum ADS_EXPORT Result {
  SUCCESS,
  FAILED
};

// CallbackHandler must not be destroyed if it has pending callbacks
class ADS_EXPORT CallbackHandler {
 public:
  virtual ~CallbackHandler() = default;

  // Category
  virtual void OnGetCategory(
      const Result /* result */,
      const std::string& /* category */,
      const std::vector<CategoryInfo>& /* ads */) {}

  virtual void OnGetSampleCategory(
      const Result /* result */,
      const std::string& /* category */) {}

  // URL Session
  virtual bool OnURLSessionReceivedResponse(
      const uint64_t /* session_id */,
      const std::string& /* url */,
      const int /* response_status_code */,
      const std::string& /* response */,
      const std::map<std::string, std::string>& /* headers */);
};

}  // namespace ads

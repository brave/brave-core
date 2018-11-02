/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <memory>

#include "bat/ads/callback_handler.h"
#include "bat/ads/url_session.h"

namespace ads {

// TODO(Terry Mancey): Refactor URLSessionCallbackHandlerCallback
// taking into consideration callback_handler.h
using URLSessionCallbackHandlerCallback = std::function<void (
  const std::string&, const int, const std::string&,
  const std::map<std::string, std::string>& headers)>;

class URLSessionCallbackHandler : public CallbackHandler {
 public:
  URLSessionCallbackHandler();
  ~URLSessionCallbackHandler() override;

  void Clear();

  bool AddCallbackHandler(
      const std::unique_ptr<URLSession> url_session,
      URLSessionCallbackHandlerCallback callback);

  bool RunCallbackHandler(
      const uint64_t session_id,
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bool OnURLSessionReceivedResponse(
      const uint64_t session_id,
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers) override;

 private:
  std::map<uint64_t, URLSessionCallbackHandlerCallback>
    url_session_callback_handlers_;
};

}  // namespace ads

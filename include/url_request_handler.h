/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "../include/callback_handler.h"
#include "../include/ads_url_loader.h"

namespace bat_ads {

class URLRequestHandler : public ads::CallbackHandler {
 public:
  using URLRequestCallback = std::function<void (bool, const std::string&,
      const std::map<std::string, std::string>& headers)>;

  URLRequestHandler();
  ~URLRequestHandler() override;

  void Clear();
  bool AddRequestHandler(std::unique_ptr<ads::AdsURLLoader> loader,
                         URLRequestCallback callback);
  bool RunRequestHandler(uint64_t request_id,
                         bool success,
                         const std::string& response,
                         const std::map<std::string, std::string>& headers);

 private:
  //  AdsCallbackHandler impl
  void OnURLRequestResponse(uint64_t request_id, const std::string& url,
      int response_code, const std::string& response,
      const std::map<std::string, std::string>& headers) override;

  std::map<uint64_t, URLRequestCallback> request_handlers_;
};

}  // namespace bat_ads

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_API_H_
#define BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_API_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class AIChatAPI {
 public:
  explicit AIChatAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  AIChatAPI(const AIChatAPI&) = delete;
  AIChatAPI& operator=(const AIChatAPI&) = delete;
  ~AIChatAPI();

  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;
  using APIRequestResult = api_request_helper::APIRequestResult;

  void QueryPrompt(ResponseCallback callback, const std::string& prompt);

 private:
  using URLRequestCallback = base::OnceCallback<void(APIRequestResult)>;

  void OnGetResponse(ResponseCallback callback,
                     APIRequestResult api_request_result);

  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<AIChatAPI> weak_ptr_factory_{this};
};

#endif  // BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_API_H_

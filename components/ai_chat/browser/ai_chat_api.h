/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_API_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_API_H_

#include <memory>
#include <string>
#include <vector>

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

  // This function queries both types of APIs: SSE and non-SSE.
  // In non-SSE cases, only the data_completed_callback will be triggered.
  void QueryPrompt(const std::string& prompt,
                   const std::vector<std::string> extra_stop_sequences,
                   api_request_helper::APIRequestHelper::ResultCallback
                       data_completed_callback,
                   api_request_helper::APIRequestHelper::DataReceivedCallback
                       data_received_callback = base::NullCallback());
  // Clears all in-progress requests
  // TODO(nullhook): Keep track of in-progress requests and cancel them
  // individually. This would be useful to keep some in-progress requests alive.
  void ClearAllQueries();

 private:
  api_request_helper::APIRequestHelper api_request_helper_;
};

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_API_H_

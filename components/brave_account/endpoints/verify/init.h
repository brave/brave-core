/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_INIT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_INIT_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account::endpoints {
class VerifyInit {
 public:
  using Result = api_request_helper::APIRequestResult;

  explicit VerifyInit(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  VerifyInit(const VerifyInit&) = delete;
  VerifyInit& operator=(const VerifyInit&) = delete;

  void Send(const std::string& email,
            api_request_helper::APIRequestHelper::ResultCallback callback);

 private:
  api_request_helper::APIRequestHelper api_request_helper_;
};
}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_INIT_H_

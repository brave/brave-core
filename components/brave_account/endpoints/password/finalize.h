/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_FINALIZE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_FINALIZE_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account::endpoints {

class PasswordFinalize {
 public:
  explicit PasswordFinalize(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  PasswordFinalize(const PasswordFinalize&) = delete;
  PasswordFinalize& operator=(const PasswordFinalize&) = delete;

  ~PasswordFinalize();

  void Send(const std::string& verification_token,
            const std::string& serialized_record,
            base::OnceCallback<void(bool)> callback);

 private:
  void OnResponse(base::OnceCallback<void(bool)> callback,
                  api_request_helper::APIRequestResult result);

  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<PasswordFinalize> weak_factory_{this};
};

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_FINALIZE_H_

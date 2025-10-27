/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_

#include <string_view>

#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/verify_result_request.h"
#include "brave/components/brave_account/endpoints/verify_result_response.h"
#include "url/gurl.h"

namespace brave_account::endpoints {

struct VerifyResult {
  using Request = VerifyResultRequest;
  using Response = VerifyResultResponse;
  using Error = Error;
  static GURL URL();
  static std::string_view Method();
};

static_assert(endpoint_client::IsEndpoint<VerifyResult>);

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_

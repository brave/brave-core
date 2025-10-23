/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_

#include "brave/components/brave_account/endpoint_client/concepts.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/verify_result_request.h"
#include "brave/components/brave_account/endpoints/verify_result_response.h"
#include "url/gurl.h"

namespace brave_account::endpoints {

struct VerifyResult {
  using Request = endpoint_client::POST<VerifyResultRequest>;
  using Response = VerifyResultResponse;
  using Error = Error;
  static GURL URL();
};

static_assert(endpoint_client::concepts::Endpoint<VerifyResult>);

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_

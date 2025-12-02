/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_

#include "brave/components/brave_account/endpoint_client/brave_endpoint.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoints/error_body.h"
#include "brave/components/brave_account/endpoints/verify_result_bodies.h"

namespace brave_account::endpoints {

using VerifyResult = endpoint_client::BraveEndpoint<
    "accounts.bsg",
    "/v2/verify/result",
    endpoint_client::POST<VerifyResultRequestBody>,
    endpoint_client::Response<VerifyResultSuccessBody, ErrorBody>>;

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_VERIFY_RESULT_H_

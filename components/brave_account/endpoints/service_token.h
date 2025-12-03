/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_SERVICE_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_SERVICE_TOKEN_H_

#include "brave/components/brave_account/endpoint_client/brave_endpoint.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/service_token_request.h"
#include "brave/components/brave_account/endpoints/service_token_response.h"

namespace brave_account::endpoints {

using ServiceToken = endpoint_client::BraveEndpoint<
    "accounts.bsg",
    "/v2/auth/service_token",
    endpoint_client::POST<ServiceTokenRequest>,
    endpoint_client::Response<ServiceTokenResponse, Error>>;

static_assert(endpoint_client::IsEndpoint<ServiceToken>);

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_SERVICE_TOKEN_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_SERVICE_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_SERVICE_TOKEN_H_

#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/host.h"
#include "brave/components/brave_account/endpoints/service_token_request.h"
#include "brave/components/brave_account/endpoints/service_token_response.h"
#include "url/gurl.h"

namespace brave_account::endpoints {

struct ServiceToken {
  using Request = endpoint_client::POST<ServiceTokenRequest>;
  using Response = ServiceTokenResponse;
  using Error = Error;

  static GURL URL() { return Host().Resolve("/v2/auth/service_token"); }
};

static_assert(endpoint_client::IsEndpoint<ServiceToken>);

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_SERVICE_TOKEN_H_

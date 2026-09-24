/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_VALIDATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_VALIDATE_H_

#include "brave/components/brave_account/endpoint_client/brave_endpoint.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoints/auth_validate_bodies.h"
#include "brave/components/brave_account/endpoints/error_body.h"

namespace brave_account::endpoints {

using AuthValidate = endpoint_client::BraveEndpoint<
    "accounts.bsg",
    "/v2/auth/validate",
    endpoint_client::GET<AuthValidateRequestBody>,
    endpoint_client::Response<AuthValidateSuccessBody, ErrorBody>>;

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_VALIDATE_H_

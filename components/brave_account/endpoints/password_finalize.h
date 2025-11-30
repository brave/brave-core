/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_FINALIZE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_FINALIZE_H_

#include "brave/components/brave_account/endpoint_client/brave_endpoint.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/password_finalize_request.h"
#include "brave/components/brave_account/endpoints/password_finalize_response.h"

namespace brave_account::endpoints {

using PasswordFinalize = endpoint_client::BraveEndpoint<
    "accounts.bsg",
    "/v2/accounts/password/finalize",
    endpoint_client::POST<PasswordFinalizeRequest>,
    endpoint_client::Response<PasswordFinalizeResponse, Error>>;

static_assert(endpoint_client::IsEndpoint<PasswordFinalize>);

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_FINALIZE_H_

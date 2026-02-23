/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_LOGOUT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_LOGOUT_H_

#include "brave/components/endpoint_client/brave_endpoint.h"
#include "brave/components/endpoint_client/request_types.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/brave_account/endpoints/auth_logout_bodies.h"

namespace brave_account::endpoints {

using AuthLogout = endpoint_client::BraveEndpoint<
    "accounts.bsg",
    "/v2/auth/logout",
    endpoint_client::POST<AuthLogoutRequestBody>,
    endpoint_client::Response<AuthLogoutSuccessBody, AuthLogoutErrorBody>>;

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_LOGOUT_H_

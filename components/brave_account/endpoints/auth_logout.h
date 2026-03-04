/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_LOGOUT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_LOGOUT_H_

#include "brave/components/brave_account/endpoint_client/brave_endpoint.h"
#include "brave/components/brave_account/endpoint_client/json_empty_body.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"

namespace brave_account::endpoints {

using AuthLogout = endpoint_client::BraveEndpoint<
    "accounts.bsg",
    "/v2/auth/logout",
    endpoint_client::POST<endpoint_client::JSONEmptyBody>,
    endpoint_client::Response<endpoint_client::JSONEmptyBody,
                              endpoint_client::JSONEmptyBody>>;

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_AUTH_LOGOUT_H_

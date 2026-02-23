/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_ENDPOINTS_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_ENDPOINTS_H_

#include "brave/components/endpoint_client/brave_endpoint.h"
#include "brave/components/endpoint_client/request_types.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/email_aliases/email_aliases_api.h"

namespace email_aliases::endpoints {

using AliasList = endpoint_client::BraveEndpoint<
    "aliases",
    "/manage?status=active",
    endpoint_client::GET<AliasListRequest>,
    endpoint_client::Response<AliasListResponse, ErrorMessage>>;

using GenerateAlias = endpoint_client::BraveEndpoint<
    "aliases",
    "/manage",
    endpoint_client::POST<GenerateAliasRequest>,
    endpoint_client::Response<GenerateAliasResponse,
                                             ErrorMessage>>;

using UpdateAlias = endpoint_client::BraveEndpoint<
    "aliases",
    "/manage",
    endpoint_client::PUT<UpdateAliasRequest>,
    endpoint_client::Response<AliasEditedResponse,
                                             ErrorMessage>>;

using DeleteAlias = endpoint_client::BraveEndpoint<
    "aliases",
    "/manage",
    endpoint_client::DELETE<DeleteAliasRequest>,
    endpoint_client::Response<AliasEditedResponse,
                                             ErrorMessage>>;

}  // namespace email_aliases::endpoints

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_ENDPOINTS_H_

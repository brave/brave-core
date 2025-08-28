/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_INIT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_INIT_H_

#include <string_view>

#include "brave/components/brave_account/endpoint_client/concepts.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/password_init_request.h"
#include "brave/components/brave_account/endpoints/password_init_response.h"
#include "url/gurl.h"

namespace brave_account::endpoints {

struct PasswordInit {
  using Request = PasswordInitRequest;
  using Response = PasswordInitResponse;
  using Error = Error;
  static GURL URL();
  static std::string_view Method();
};

static_assert(endpoint_client::concepts::Endpoint<PasswordInit>);

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_PASSWORD_INIT_H_

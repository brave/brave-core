/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_BRAVE_ENDPOINT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_BRAVE_ENDPOINT_H_

#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_account/endpoint_client/is_request.h"
#include "brave/components/brave_account/endpoint_client/is_response.h"
#include "brave/components/brave_account/endpoint_client/static_string.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_account::endpoint_client {

template <detail::StaticString Prefix,
          detail::StaticString Path,
          detail::IsRequest RequestT,
          detail::IsResponse ResponseT>
struct BraveEndpoint {
  using Request = RequestT;
  using Response = ResponseT;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              brave_domains::GetServicesDomain(Prefix.value)}))
        .Resolve(Path.value);
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_BRAVE_ENDPOINT_H_

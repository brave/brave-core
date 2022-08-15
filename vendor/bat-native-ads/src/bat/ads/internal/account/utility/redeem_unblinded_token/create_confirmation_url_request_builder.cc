/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token_util.h"
#include "bat/ads/internal/server/headers/via_header_util.h"
#include "url/gurl.h"

namespace ads {

CreateConfirmationUrlRequestBuilder::CreateConfirmationUrlRequestBuilder(
    const ConfirmationInfo& confirmation)
    : confirmation_(confirmation) {
  DCHECK(confirmation_.IsValid());
}

CreateConfirmationUrlRequestBuilder::~CreateConfirmationUrlRequestBuilder() =
    default;

// POST /v2/confirmation/{confirmation_id}/{credential}

mojom::UrlRequestInfoPtr CreateConfirmationUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->headers = BuildHeaders();
  url_request->content = BuildBody();
  url_request->content_type = "application/json";
  url_request->method = mojom::UrlRequestMethodType::kPost;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL CreateConfirmationUrlRequestBuilder::BuildUrl() const {
  std::string spec = base::StringPrintf(
      "%s/v2/confirmation/%s", GetAnonymousHost(confirmation_.ad_type).c_str(),
      confirmation_.id.c_str());

  if (!confirmation_.credential.empty()) {
    spec += "/";
    spec += confirmation_.credential;
  }

  return GURL(spec);
}

std::vector<std::string> CreateConfirmationUrlRequestBuilder::BuildHeaders()
    const {
  std::vector<std::string> headers;

  const std::string via_header = server::BuildViaHeader();
  headers.push_back(via_header);

  const std::string accept_header = "accept: application/json";
  headers.push_back(accept_header);

  return headers;
}

std::string CreateConfirmationUrlRequestBuilder::BuildBody() const {
  return CreateConfirmationRequestDTO(confirmation_);
}

}  // namespace ads

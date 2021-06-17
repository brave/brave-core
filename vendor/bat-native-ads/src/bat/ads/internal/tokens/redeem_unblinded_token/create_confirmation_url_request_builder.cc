/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include "base/base64.h"
#include "base/base64url.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_util.h"
#include "bat/ads/internal/locale/country_code_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/server/confirmations_server_util.h"
#include "bat/ads/internal/server/via_header_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

CreateConfirmationUrlRequestBuilder::CreateConfirmationUrlRequestBuilder(
    const ConfirmationInfo& confirmation)
    : confirmation_(confirmation) {
  DCHECK(confirmation_.IsValid());
}

CreateConfirmationUrlRequestBuilder::~CreateConfirmationUrlRequestBuilder() =
    default;

// POST /v1/confirmation/{confirmation_id}/{credential}

UrlRequestPtr CreateConfirmationUrlRequestBuilder::Build() {
  UrlRequestPtr url_request = UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->headers = BuildHeaders();
  url_request->content = BuildBody();
  url_request->content_type = "application/json";
  url_request->method = UrlRequestMethod::POST;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string CreateConfirmationUrlRequestBuilder::BuildUrl() const {
  std::string url = base::StringPrintf("%s/v1/confirmation/%s",
                                       confirmations::server::GetHost().c_str(),
                                       confirmation_.id.c_str());

  if (!confirmation_.credential.empty()) {
    url += "/";
    url += confirmation_.credential;
  }

  return url;
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

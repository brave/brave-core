/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include "base/base64.h"
#include "base/base64url.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "wrapper.hpp"
#include "bat/ads/internal/locale/country_code_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/internal/server/redeem_unblinded_token/create_confirmation_util.h"

namespace ads {

using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;
using challenge_bypass_ristretto::TokenPreimage;

CreateConfirmationUrlRequestBuilder::CreateConfirmationUrlRequestBuilder(
    const ConfirmationInfo& confirmation)
    : confirmation_(confirmation) {
  DCHECK(confirmation_.IsValid());
}

CreateConfirmationUrlRequestBuilder::
~CreateConfirmationUrlRequestBuilder() = default;

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
  return base::StringPrintf("%s/v1/confirmation/%s/%s",
      server::GetDomain().c_str(), confirmation_.id.c_str(),
          confirmation_.credential.c_str());
}

std::vector<std::string>
CreateConfirmationUrlRequestBuilder::BuildHeaders() const {
  return {
    "accept: application/json"
  };
}

std::string CreateConfirmationUrlRequestBuilder::BuildBody() const {
  return CreateConfirmationRequestDTO(confirmation_);
}

}  // namespace ads

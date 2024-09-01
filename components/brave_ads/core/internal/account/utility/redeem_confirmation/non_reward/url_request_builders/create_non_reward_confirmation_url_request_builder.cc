/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/payload/confirmation_payload_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::vector<std::string> BuildHeaders() {
  return {"accept: application/json"};
}

}  // namespace

CreateNonRewardConfirmationUrlRequestBuilder::
    CreateNonRewardConfirmationUrlRequestBuilder(ConfirmationInfo confirmation)
    : confirmation_(std::move(confirmation)) {
  CHECK(IsValid(confirmation_));
}

mojom::UrlRequestInfoPtr CreateNonRewardConfirmationUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr mojom_url_request = mojom::UrlRequestInfo::New();
  mojom_url_request->url = BuildUrl();
  mojom_url_request->headers = BuildHeaders();
  mojom_url_request->content = BuildBody();
  mojom_url_request->content_type = "application/json";
  mojom_url_request->method = mojom::UrlRequestMethodType::kPost;

  return mojom_url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL CreateNonRewardConfirmationUrlRequestBuilder::BuildUrl() const {
  const std::string url_host = confirmation_.ad_type == AdType::kSearchResultAd
                                   ? GetAnonymousSearchUrlHost()
                                   : GetAnonymousUrlHost();

  const std::string spec = base::StrCat(
      {url_host,
       BuildCreateNonRewardConfirmationUrlPath(confirmation_.transaction_id)});

  return GURL(spec);
}

std::string CreateNonRewardConfirmationUrlRequestBuilder::BuildBody() const {
  return json::writer::WriteConfirmationPayload(confirmation_);
}

}  // namespace brave_ads

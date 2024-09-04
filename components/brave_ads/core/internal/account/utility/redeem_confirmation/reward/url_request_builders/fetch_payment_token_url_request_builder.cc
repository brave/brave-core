/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

FetchPaymentTokenUrlRequestBuilder::FetchPaymentTokenUrlRequestBuilder(
    ConfirmationInfo confirmation)
    : confirmation_(std::move(confirmation)) {
  CHECK(IsValid(confirmation_));
}

mojom::UrlRequestInfoPtr FetchPaymentTokenUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr mojom_url_request = mojom::UrlRequestInfo::New();
  mojom_url_request->url = BuildUrl();
  mojom_url_request->method = mojom::UrlRequestMethodType::kGet;

  return mojom_url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL FetchPaymentTokenUrlRequestBuilder::BuildUrl() const {
  const std::string url_host =
      confirmation_.ad_type == mojom::AdType::kSearchResultAd
          ? GetAnonymousSearchUrlHost()
          : GetAnonymousUrlHost();

  const std::string spec = base::StrCat(
      {url_host, BuildFetchPaymentTokenUrlPath(confirmation_.transaction_id)});

  return GURL(spec);
}

}  // namespace brave_ads

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/network/oblivious_http_client_impl.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

ObliviousHttpClientImpl::ObliviousHttpClientImpl(GURL url,
                                                 SendRequestCallback callback)
    : url_(std::move(url)), callback_(std::move(callback)) {}

ObliviousHttpClientImpl::~ObliviousHttpClientImpl() = default;

void ObliviousHttpClientImpl::OnCompleted(
    network::mojom::ObliviousHttpCompletionResultPtr response) {
  CHECK(response);

  auto mojom_url_response = mojom::UrlResponseInfo::New();

  mojom_url_response->url = url_;

  switch (response->which()) {
    case network::mojom::ObliviousHttpCompletionResult::Tag::kNetError:
      mojom_url_response->code = response->get_net_error();
      break;
    case network::mojom::ObliviousHttpCompletionResult::Tag::
        kOuterResponseErrorCode:
      mojom_url_response->code = response->get_outer_response_error_code();
      break;
    case network::mojom::ObliviousHttpCompletionResult::Tag::kInnerResponse: {
      const auto& inner_response = response->get_inner_response();
      mojom_url_response->code = inner_response->response_code;
      mojom_url_response->body = inner_response->response_body;
      break;
    }
  }

  std::move(callback_).Run(std::move(mojom_url_response));
}

}  // namespace brave_ads

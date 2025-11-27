/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/oblivious_http_client_impl.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

ObliviousHttpClientImpl::ObliviousHttpClientImpl(GURL url,
                                                 UrlRequestCallback callback)
    : url_(std::move(url)), callback_(std::move(callback)) {}

ObliviousHttpClientImpl::~ObliviousHttpClientImpl() = default;

void ObliviousHttpClientImpl::OnCompleted(
    network::mojom::ObliviousHttpCompletionResultPtr response) {
  CHECK(response);

  auto mojom_url_response = mojom::UrlResponseInfo::New();

  mojom_url_response->url = url_;

  if (response->is_net_error()) {
    mojom_url_response->code = response->get_net_error();
  } else if (response->is_outer_response_error_code()) {
    mojom_url_response->code = response->get_outer_response_error_code();
  } else if (response->is_inner_response()) {
    const auto& inner_response = response->get_inner_response();
    mojom_url_response->code = inner_response->response_code;
    mojom_url_response->body = inner_response->response_body;
  }

  std::move(callback_).Run(std::move(mojom_url_response));
}

}  // namespace brave_ads

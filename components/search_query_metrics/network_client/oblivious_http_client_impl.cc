/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/network_client/oblivious_http_client_impl.h"

#include <cstdint>
#include <string>
#include <utility>

#include "base/check.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace metrics {

ObliviousHttpClientImpl::ObliviousHttpClientImpl(GURL url,
                                                 SendRequestCallback callback)
    : url_(std::move(url)), callback_(std::move(callback)) {}

ObliviousHttpClientImpl::~ObliviousHttpClientImpl() = default;

void ObliviousHttpClientImpl::OnCompleted(
    network::mojom::ObliviousHttpCompletionResultPtr response) {
  CHECK(response);

  int32_t response_code = net::ERR_UNEXPECTED;
  std::string response_body;

  if (response->is_net_error()) {
    response_code = response->get_net_error();
  } else if (response->is_outer_response_error_code()) {
    response_code = response->get_outer_response_error_code();
  } else if (response->is_inner_response()) {
    const auto& inner_response = response->get_inner_response();
    response_code = inner_response->response_code;
    response_body = inner_response->response_body;
  }

  std::move(callback_).Run(url_, response_code, response_body, /*headers=*/{});
}

}  // namespace metrics

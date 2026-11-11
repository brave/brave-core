/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_OPTIONS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_OPTIONS_H_

#include <cstddef>
#include <optional>

#include "base/time/time.h"
#include "brave/components/endpoint_client/retry_options.h"
#include "brave/components/endpoint_client/url_replacements.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom.h"

namespace endpoint_client::internal {

struct RequestOptions {
  std::optional<std::size_t> max_body_size =
      network::SimpleURLLoader::kMaxBoundedStringDownloadSize;
  net::MutableNetworkTrafficAnnotationTag network_traffic_annotation_tag;
  network::mojom::RedirectMode redirect_mode =
      network::mojom::RedirectMode::kError;
  RetryOptions retry_options;
  base::TimeDelta timeout_duration;
  UrlReplacements url_replacements;
};

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_OPTIONS_H_

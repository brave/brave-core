/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/common/web_ui_loading_util.h"

#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace content::webui {
namespace {

network::mojom::URLResponseHeadPtr UseContentLengthFromHeaders(
    network::mojom::URLResponseHeadPtr headers) {
  if (auto content_length = headers->headers->GetContentLength();
      content_length != -1) {
    headers->content_length = content_length;
  }
  return headers;
}

}  // namespace
}  // namespace content::webui

#define OnReceiveResponse(headers, ...) \
  OnReceiveResponse(UseContentLengthFromHeaders(headers), __VA_ARGS__)

#include "src/content/common/web_ui_loading_util.cc"
#undef OnReceiveResponse

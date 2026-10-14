/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
namespace {

url::Origin MaybeTreatCrossOriginAsOpaque(const url::Origin& origin,
                                          const GURL& url) {
  // A cross-origin WebSocket opened by a .onion page must not disclose the
  // address it came from, so it is given an opaque origin.
  return net::IsCrossOriginOnionRequest(origin, url)
             ? origin.DeriveNewOpaqueOrigin()
             : origin;
}

}  // namespace
}  // namespace content

#include <content/browser/websockets/websocket_connector_impl.cc>

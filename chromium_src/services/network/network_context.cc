/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/notreached.h"
#include "net/url_request/url_request_context.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace network {
namespace {

// Converts NetworkContextParams.tor_mode for the request context, which the
// host resolver reads to block .onion lookups. Called from the plaster in the
// NetworkContext constructor.
net::URLRequestContext::TorMode ToURLRequestContextTorMode(
    mojom::TorMode tor_mode) {
  switch (tor_mode) {
    case mojom::TorMode::kEnabled:
      return net::URLRequestContext::TorMode::kEnabled;
    case mojom::TorMode::kDisabled:
      return net::URLRequestContext::TorMode::kDisabled;
  }
  NOTREACHED();
}

}  // namespace
}  // namespace network

#include <services/network/network_context.cc>

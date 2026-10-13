/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <services/network/network_context.cc>

namespace network {

// Brave addition to mojom::NetworkContext. Applies runtime changes of the
// "Only resolve .onion addresses in Tor windows" pref to the request context,
// which the host resolver reads to block .onion lookups.
void NetworkContext::SetOnionOnlyInTorWindows(bool onion_only) {
  url_request_context_->set_onion_only_in_tor_windows(onion_only);
}

}  // namespace network

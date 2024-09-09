/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/devtools/protocol/network_handler.h"

#include "src/content/browser/devtools/protocol/network_handler.cc"

namespace content::protocol {

void NetworkHandler::RequestAdblockInfoReceived(
    const std::string& request_id,
    std::unique_ptr<protocol::Network::AdblockInfo> info) {
  if (!enabled_) {
    return;
  }

  frontend_->RequestAdblockInfoReceived(request_id, std::move(info));
}

}  // namespace content::protocol

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"

namespace brave_wallet {

SnapRequestHandlerImpl::SnapRequestHandlerImpl() = default;
SnapRequestHandlerImpl::~SnapRequestHandlerImpl() = default;

void SnapRequestHandlerImpl::Bind(
    mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void SnapRequestHandlerImpl::HandleSnapRequest(
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    HandleSnapRequestCallback callback) {
  // Minimal extraction: snap.request() is acknowledged but not processed.
  // In a full implementation this routes to KeyringService, state storage,
  // permission controller, etc.
  std::move(callback).Run(
      std::nullopt, "snap.request() not implemented in minimal extraction");
}

}  // namespace brave_wallet

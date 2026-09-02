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
  if (request_observer_for_testing_) {
    request_observer_for_testing_.Run(snap_id, method, params);
  }

  if (method == "snap_manageState") {
    HandleManageState(snap_id, std::move(params), std::move(callback));
    return;
  }

  std::move(callback).Run(std::nullopt, "Unsupported method: " + method);
}

void SnapRequestHandlerImpl::HandleManageState(
    const std::string& snap_id,
    base::Value params,
    HandleSnapRequestCallback callback) {
  if (!params.is_dict()) {
    std::move(callback).Run(std::nullopt, "Unsupported operation");
    return;
  }
  const std::string* operation = params.GetDict().FindString("operation");
  if (!operation) {
    std::move(callback).Run(std::nullopt, "Unsupported operation");
    return;
  }

  if (*operation == "update") {
    const base::DictValue* new_state = params.GetDict().FindDict("newState");
    if (!new_state) {
      std::move(callback).Run(std::nullopt, "Unsupported operation");
      return;
    }
    snap_state_[snap_id] = new_state->Clone();
    std::move(callback).Run(base::Value(), std::nullopt);
    return;
  }

  if (*operation == "get") {
    auto it = snap_state_.find(snap_id);
    std::move(callback).Run(it != snap_state_.end()
                                ? base::Value(it->second.Clone())
                                : base::Value(),
                            std::nullopt);
    return;
  }

  if (*operation == "clear") {
    snap_state_.erase(snap_id);
    std::move(callback).Run(base::Value(), std::nullopt);
    return;
  }

  std::move(callback).Run(std::nullopt, "Unsupported operation");
}

void SnapRequestHandlerImpl::SetRequestObserverForTesting(
    RequestObserverForTesting observer) {
  request_observer_for_testing_ = std::move(observer);
}

}  // namespace brave_wallet

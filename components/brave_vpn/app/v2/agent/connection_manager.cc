/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

#include <utility>

#include "base/notimplemented.h"

namespace brave_vpn::v2 {

ConnectionManager::ConnectionManager() = default;

ConnectionManager::~ConnectionManager() = default;

void ConnectionManager::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void ConnectionManager::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

bool ConnectionManager::HasObservers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !observers_.empty();
}

mojom::VpnConnectionState ConnectionManager::connection_state() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return connection_state_;
}

const std::optional<std::string>& ConnectionManager::error_message() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return error_message_;
}

void ConnectionManager::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::ResetConnectionState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::SetConnectionState(
    mojom::VpnConnectionState state,
    std::optional<std::string> error_message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == state && error_message_ == error_message) {
    return;
  }
  connection_state_ = state;
  error_message_ = std::move(error_message);

  observers_.Notify(&Observer::OnConnectionStateChanged, connection_state_,
                    error_message_);
}

}  // namespace brave_vpn::v2

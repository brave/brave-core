/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_api.h"

#include "base/check_deref.h"

namespace brave_vpn::v2 {

ConnectionApi::ConnectionApi(Delegate* delegate)
    : delegate_(CHECK_DEREF(delegate)) {}

ConnectionApi::~ConnectionApi() = default;

void ConnectionApi::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ConnectImpl();
}

void ConnectionApi::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DisconnectImpl();
}

void ConnectionApi::CheckConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CheckConnectionImpl();
}

}  // namespace brave_vpn::v2

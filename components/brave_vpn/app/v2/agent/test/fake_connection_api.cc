/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_api.h"

#include "base/check_deref.h"

namespace brave_vpn::v2 {

FakeConnectionApi::FakeConnectionApi(Delegate* delegate, Log* log)
    : ConnectionApi(delegate), log_(CHECK_DEREF(log)) {
  ++log_->created;
  log_->current = this;
}

FakeConnectionApi::~FakeConnectionApi() {
  ++log_->destroyed;
  if (log_->current == this) {
    log_->current = nullptr;
  }
}

void FakeConnectionApi::ReportEstablished() {
  delegate().OnConnectionEstablished();
}

void FakeConnectionApi::ReportTearingDown() {
  delegate().OnConnectionTearingDown();
}

void FakeConnectionApi::ReportTerminated() {
  delegate().OnConnectionTerminated();
}

void FakeConnectionApi::ReportFailed(FailureReason reason) {
  delegate().OnConnectionFailed(reason);
}

void FakeConnectionApi::ConnectImpl() {
  ++log_->connect_requests;
}

void FakeConnectionApi::DisconnectImpl() {
  ++log_->disconnect_requests;
}

void FakeConnectionApi::CheckConnectionImpl() {
  ++log_->check_requests;
}

}  // namespace brave_vpn::v2

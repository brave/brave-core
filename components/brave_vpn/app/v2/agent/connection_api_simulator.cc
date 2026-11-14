/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_api_simulator.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"

namespace brave_vpn::v2 {

ConnectionApiSimulator::ConnectionApiSimulator(Delegate* delegate)
    : ConnectionApi(delegate) {}

ConnectionApiSimulator::~ConnectionApiSimulator() = default;

void ConnectionApiSimulator::ConnectImpl() {
  CancelPendingTransitions();
  const bool success = base::RandIntInclusive(0, 9) > 3;
  if (success) {
    transition_timer_.Start(FROM_HERE, base::Seconds(2), this,
                            &ConnectionApiSimulator::NotifyEstablished);
  } else {
    transition_timer_.Start(FROM_HERE, base::Seconds(1), this,
                            &ConnectionApiSimulator::NotifyFailed);
  }
}

void ConnectionApiSimulator::DisconnectImpl() {
  CancelPendingTransitions();
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectionApiSimulator::NotifyTearingDown,
                                weak_factory_.GetWeakPtr()));
  transition_timer_.Start(FROM_HERE, base::Seconds(1), this,
                          &ConnectionApiSimulator::NotifyTerminated);
}

void ConnectionApiSimulator::CheckConnectionImpl() {
  if (connection_alive_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&ConnectionApiSimulator::NotifyEstablished,
                                  weak_factory_.GetWeakPtr()));
  } else {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&ConnectionApiSimulator::NotifyTerminated,
                                  weak_factory_.GetWeakPtr()));
  }
}

void ConnectionApiSimulator::CancelPendingTransitions() {
  connection_alive_ = false;
  transition_timer_.Stop();
  weak_factory_.InvalidateWeakPtrs();
}

void ConnectionApiSimulator::NotifyEstablished() {
  connection_alive_ = true;
  delegate().OnConnectionEstablished();
}

void ConnectionApiSimulator::NotifyTearingDown() {
  delegate().OnConnectionTearingDown();
}

void ConnectionApiSimulator::NotifyTerminated() {
  delegate().OnConnectionTerminated();
}

void ConnectionApiSimulator::NotifyFailed() {
  delegate().OnConnectionFailed(FailureReason::kServerUnreachable);
}

}  // namespace brave_vpn::v2

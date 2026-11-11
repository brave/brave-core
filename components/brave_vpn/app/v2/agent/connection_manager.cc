/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_vpn::v2 {
namespace {

bool IsRetryableConnectionFailure(ConnectionApi::FailureReason reason) {
  switch (reason) {
    case ConnectionApi::FailureReason::kServerUnreachable:
      return true;
    case ConnectionApi::FailureReason::kInternalError:
      return false;
  }
  NOTREACHED();
}

// The state that reports a failure to the user.
mojom::VpnConnectionState FailureStateFor(ConnectionApi::FailureReason reason) {
  switch (reason) {
    case ConnectionApi::FailureReason::kServerUnreachable:
      return mojom::VpnConnectionState::kConnectFailed;
    case ConnectionApi::FailureReason::kInternalError:
      return mojom::VpnConnectionState::kInternalError;
  }
  NOTREACHED();
}
}  // namespace

ConnectionManager::ConnectionManager(ConnectionApiFactory api_factory)
    : connection_api_factory_(std::move(api_factory)),
      network_available_(!net::NetworkChangeNotifier::IsOffline()),
      reconnect_backoff_(&kReconnectBackoffPolicy) {
  CHECK(connection_api_factory_);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

ConnectionManager::~ConnectionManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
  // Destroy the API while every other member is still alive. ConnectionApi
  // implementations must not call the delegate from their destructor.
  connection_api_.reset();
}

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

const mojom::VpnRegion& ConnectionManager::current_region() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return *current_region_;
}

bool ConnectionManager::auto_reconnect_enabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auto_reconnect_enabled_;
}

bool ConnectionManager::smart_proxy_routing_enabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return smart_proxy_routing_enabled_;
}

bool ConnectionManager::allow_lan_traffic_available() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_lan_traffic_available_;
}

bool ConnectionManager::allow_lan_traffic_enabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_lan_traffic_enabled_;
}

const std::string& ConnectionManager::last_hostname() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return last_hostname_;
}

void ConnectionManager::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Starts a fresh retry sequence on a user-initiated connect.
  desired_connected_ = true;
  reconnect_backoff_.Reset();

  switch (connection_state_) {
    case mojom::VpnConnectionState::kConnecting:
    case mojom::VpnConnectionState::kConnected:
      return;
    case mojom::VpnConnectionState::kDisconnecting:
      // The API is busy taking the tunnel down. OnSessionFinished() connects
      // once that is done.
      connect_after_teardown_ = true;
      return;
    case mojom::VpnConnectionState::kWaitingRetry:
      // Nothing is in flight while waiting; don't make the user wait out the
      // timer.
    case mojom::VpnConnectionState::kDisconnected:
    case mojom::VpnConnectionState::kConnectFailed:
    case mojom::VpnConnectionState::kInternalError:
      StartSession();
      return;
  }
}

void ConnectionManager::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  desired_connected_ = false;
  connect_after_teardown_ = false;
  reconnect_timer_.Stop();
  reconcile_timer_.Stop();
  connection_stable_timer_.Stop();
  reconnect_backoff_.Reset();

  switch (connection_state_) {
    case mojom::VpnConnectionState::kDisconnected:
      return;
    case mojom::VpnConnectionState::kDisconnecting:
      // Already going down; OnSessionFinished() finishes the job.
      return;
    case mojom::VpnConnectionState::kWaitingRetry:
    case mojom::VpnConnectionState::kConnectFailed:
    case mojom::VpnConnectionState::kInternalError:
      // There is no tunnel to tear down in these states.
      UpdateConnectionState(mojom::VpnConnectionState::kDisconnected,
                            std::nullopt);
      return;
    case mojom::VpnConnectionState::kConnecting:
    case mojom::VpnConnectionState::kConnected:
      if (!connection_api_) {
        // No API instance means there is no tunnel either.
        UpdateConnectionState(mojom::VpnConnectionState::kDisconnected,
                              std::nullopt);
        return;
      }
      BeginTransition(mojom::VpnConnectionState::kDisconnecting);
      connection_api_->Disconnect();
      return;
  }
}

void ConnectionManager::ResetConnectionState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == mojom::VpnConnectionState::kConnectFailed ||
      connection_state_ == mojom::VpnConnectionState::kInternalError) {
    UpdateConnectionState(mojom::VpnConnectionState::kDisconnected,
                          std::nullopt);
  }
}

void ConnectionManager::GetAllRegions(GetAllRegionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), std::vector<mojom::VpnRegionPtr>()));
}

void ConnectionManager::SetSelectedRegion(mojom::VpnRegionPtr region) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::SetAutoReconnectState(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/59939)
  // Persist the setting once agent-side storage exists.
  UpdateAutoReconnectState(enable);
  // A connect the user asked for during a teardown is not a retry, so the
  // setting doesn't cancel it.
  if (!enable && !connect_after_teardown_ &&
      connection_state_ == mojom::VpnConnectionState::kWaitingRetry) {
    desired_connected_ = false;
    reconnect_timer_.Stop();
    reconnect_backoff_.Reset();
    UpdateConnectionState(mojom::VpnConnectionState::kDisconnected,
                          std::nullopt);
  }
}

void ConnectionManager::SetSmartProxyRoutingState(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/59939)
  // Persist the setting once agent-side storage exists.
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::SetAllowLanTrafficState(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/59939)
  // Persist the setting once agent-side storage exists.
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::UpdateConnectionState(
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

void ConnectionManager::UpdateSelectedRegion(mojom::VpnRegionPtr region) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(region);
  if (current_region_->Equals(*region)) {
    return;
  }
  current_region_ = std::move(region);
  observers_.Notify(&Observer::OnSelectedRegionChanged, *current_region_);
}

void ConnectionManager::UpdateAutoReconnectState(bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto_reconnect_enabled_ == enabled) {
    return;
  }
  auto_reconnect_enabled_ = enabled;
  observers_.Notify(&Observer::OnAutoReconnectStateChanged, enabled);
}

void ConnectionManager::UpdateSmartProxyRoutingState(bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (smart_proxy_routing_enabled_ == enabled) {
    return;
  }
  smart_proxy_routing_enabled_ = enabled;
  observers_.Notify(&Observer::OnSmartProxyRoutingStateChanged, enabled);
}

void ConnectionManager::UpdateAllowLanTrafficState(bool available,
                                                   bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (allow_lan_traffic_available_ == available &&
      allow_lan_traffic_enabled_ == enabled) {
    return;
  }
  allow_lan_traffic_available_ = available;
  allow_lan_traffic_enabled_ = enabled;
  observers_.Notify(&Observer::OnAllowLanTrafficStateChanged, available,
                    enabled);
}

void ConnectionManager::UpdateLastHostname(std::string hostname) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (last_hostname_ == hostname) {
    return;
  }
  last_hostname_ = std::move(hostname);
  observers_.Notify(&Observer::OnLastHostnameChanged, last_hostname_);
}

void ConnectionManager::OnConnectionEstablished() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ != mojom::VpnConnectionState::kConnecting) {
    // In connected state, this is the answer to CheckConnection(): the tunnel
    // is fine. In disconnecting state the attempt completed just before
    // Disconnect() reached the API; the result of the teardown follows.
    // Anywhere else no tunnel is expected.
    return;
  }
  transition_watchdog_.Stop();
  transition_settle_until_ = base::TimeTicks::Now() + kTransitionGracePeriod;
  // The backoff is deliberately not reset here, so that a tunnel which comes
  // up and drops straight away doesn't retry at full speed forever.
  connection_stable_timer_.Start(
      FROM_HERE, kStableConnectionPeriod,
      base::BindOnce(&ConnectionManager::OnConnectionStable,
                     base::Unretained(this)));
  UpdateConnectionState(mojom::VpnConnectionState::kConnected, std::nullopt);
}

void ConnectionManager::OnConnectionTearingDown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ != mojom::VpnConnectionState::kConnected) {
    // A failed attempt being cleaned up stays in connecting state until the API
    // reports how it ended. A teardown we asked for is already shown.
    return;
  }
  // The tunnel is going down on its own. OnSessionFinished() decides what
  // happens next.
  connection_stable_timer_.Stop();
  reconcile_timer_.Stop();
  BeginTransition(mojom::VpnConnectionState::kDisconnecting);
}

void ConnectionManager::OnConnectionTerminated() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (connection_state_) {
    case mojom::VpnConnectionState::kConnecting:
      // The attempt ended without ever producing a tunnel.
      OnSessionFinished(mojom::VpnConnectionState::kConnectFailed,
                        /*retryable=*/true);
      return;
    case mojom::VpnConnectionState::kConnected:
      // The tunnel dropped, or CheckConnection() found it dead.
    case mojom::VpnConnectionState::kDisconnecting:
      OnSessionFinished(mojom::VpnConnectionState::kDisconnected,
                        /*retryable=*/true);
      return;
    case mojom::VpnConnectionState::kDisconnected:
    case mojom::VpnConnectionState::kWaitingRetry:
    case mojom::VpnConnectionState::kConnectFailed:
    case mojom::VpnConnectionState::kInternalError:
      // There was no tunnel to lose, so nothing has ended.
      return;
  }
}

void ConnectionManager::OnConnectionFailed(
    ConnectionApi::FailureReason reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (connection_state_) {
    case mojom::VpnConnectionState::kConnecting:
    case mojom::VpnConnectionState::kConnected:
    case mojom::VpnConnectionState::kDisconnecting:
      OnSessionFinished(FailureStateFor(reason),
                        IsRetryableConnectionFailure(reason));
      return;
    case mojom::VpnConnectionState::kDisconnected:
    case mojom::VpnConnectionState::kWaitingRetry:
    case mojom::VpnConnectionState::kConnectFailed:
    case mojom::VpnConnectionState::kInternalError:
      // Nothing was in progress, so nothing has failed.
      return;
  }
}

void ConnectionManager::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // NetworkChangeNotifier sends CONNECTION_NONE right before every online
  // notification, even when the network never went away, so this branch must
  // not do anything the online branch can't undo.
  if (type == net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE) {
    // Keep the tunnel and the user's intent: VPN can survive a short outage;
    // just stop doing work while offline.
    network_available_ = false;
    // The notifier still reports a network for the CONNECTION_NONE that only
    // announces a change, so this tells a real outage from that one.
    if (net::NetworkChangeNotifier::IsOffline()) {
      network_was_lost_ = true;
    }
    reconcile_timer_.Stop();
    // A connect the user asked for during a teardown doesn't wait for the
    // network, unlike a retry.
    if (!connect_after_teardown_) {
      reconnect_timer_.Stop();
    }
    return;
  }

  network_available_ = true;
  const bool outage_ended = std::exchange(network_was_lost_, false);

  switch (connection_state_) {
    case mojom::VpnConnectionState::kWaitingRetry:
      if (connect_after_teardown_) {
        // The attempt is already about to start; don't push it back.
        return;
      }
      // A new network is a fresh start; don't make the user wait out a long
      // backoff accumulated on the old one. A notification right after our
      // own attempt is not a new network, though: it is the tunnel going
      // away, and counting it would keep the backoff from ever growing. The
      // end of a real outage always counts, since the tunnel going away
      // doesn't take the network down.
      if (outage_ended || !IsTransitionSettling()) {
        reconnect_backoff_.Reset();
      }
      // CONNECTION_NONE stopped the timer, so start it in either case.
      ArmRetryTimer(std::max(kNetworkSettleDelay,
                             reconnect_backoff_.GetTimeUntilRelease()));
      return;
    case mojom::VpnConnectionState::kConnected:
      // Changes during the grace period are most likely caused by our own
      // tunnel, but a real one can't be told apart, so the check is put off
      // until the period is over rather than skipped.
      ScheduleReconcile(
          std::max(kNetworkSettleDelay,
                   transition_settle_until_ - base::TimeTicks::Now()));
      return;
    case mojom::VpnConnectionState::kConnecting:
    case mojom::VpnConnectionState::kDisconnecting:
      // The transition reports its own result.
    case mojom::VpnConnectionState::kDisconnected:
    case mojom::VpnConnectionState::kConnectFailed:
    case mojom::VpnConnectionState::kInternalError:
      // There is no tunnel to check.
      return;
  }
}

ConnectionApi* ConnectionManager::GetOrCreateConnectionApi() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_api_) {
    connection_api_ = connection_api_factory_.Run(this);
    CHECK(connection_api_);
  }
  return connection_api_.get();
}

void ConnectionManager::StartSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  connect_after_teardown_ = false;
  reconnect_timer_.Stop();
  BeginTransition(mojom::VpnConnectionState::kConnecting);
  GetOrCreateConnectionApi()->Connect();
}

void ConnectionManager::BeginTransition(mojom::VpnConnectionState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(state == mojom::VpnConnectionState::kConnecting ||
        state == mojom::VpnConnectionState::kDisconnecting);
  transition_watchdog_.Start(
      FROM_HERE, kTransitionTimeout,
      base::BindOnce(&ConnectionManager::OnTransitionTimedOut,
                     base::Unretained(this)));
  UpdateConnectionState(state, std::nullopt);
}

bool ConnectionManager::IsTransitionSettling() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::TimeTicks::Now() < transition_settle_until_;
}

void ConnectionManager::OnSessionFinished(mojom::VpnConnectionState end_state,
                                          bool retryable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(end_state == mojom::VpnConnectionState::kDisconnected ||
        end_state == mojom::VpnConnectionState::kConnectFailed ||
        end_state == mojom::VpnConnectionState::kInternalError);
  transition_watchdog_.Stop();
  connection_stable_timer_.Stop();
  reconcile_timer_.Stop();
  transition_settle_until_ = base::TimeTicks::Now() + kTransitionGracePeriod;

  if (!desired_connected_) {
    // The user disconnected. However the API reported the end, including a
    // teardown that ran into an error, this is a plain disconnect.
    UpdateConnectionState(mojom::VpnConnectionState::kDisconnected,
                          std::nullopt);
    return;
  }

  if (connect_after_teardown_) {
    // Connect() arrived while the tunnel was going down. This is not a failure,
    // so it doesn't count towards the backoff. The attempt starts from the
    // timer so that the API isn't called from its own callback. The flag stays
    // set until StartSession(), so that nothing mistakes the pending attempt
    // for a retry in the meantime.
    UpdateConnectionState(mojom::VpnConnectionState::kWaitingRetry,
                          std::nullopt);
    reconnect_timer_.Start(FROM_HERE, base::TimeDelta(),
                           base::BindOnce(&ConnectionManager::StartSession,
                                          base::Unretained(this)));
    return;
  }

  if (auto_reconnect_enabled_ && retryable) {
    reconnect_backoff_.InformOfRequest(/*succeeded=*/false);
    if (reconnect_backoff_.failure_count() <= kMaxReconnectAttempts) {
      UpdateConnectionState(mojom::VpnConnectionState::kWaitingRetry,
                            std::nullopt);
      ArmRetryTimer(reconnect_backoff_.GetTimeUntilRelease());
      return;
    }
    // Ran out of attempts; that's a failure whichever callback reported the
    // last one.
    if (end_state == mojom::VpnConnectionState::kDisconnected) {
      end_state = mojom::VpnConnectionState::kConnectFailed;
    }
  }

  desired_connected_ = false;
  reconnect_backoff_.Reset();
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Turn ConnectionApi::FailureReason into an error description.
  UpdateConnectionState(end_state, std::nullopt);
}

void ConnectionManager::OnTransitionTimedOut() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // The API never reported how the transition ended. Destroying the instance
  // stops whatever it was doing and guarantees that a late callback can't
  // arrive; the next attempt gets a fresh one.
  connection_api_.reset();
  OnSessionFinished(mojom::VpnConnectionState::kConnectFailed,
                    /*retryable=*/true);
}

void ConnectionManager::OnConnectionStable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  reconnect_backoff_.Reset();
}

void ConnectionManager::ArmRetryTimer(base::TimeDelta delay) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ != mojom::VpnConnectionState::kWaitingRetry ||
      !network_available_) {
    // OnNetworkChanged() arms the timer once the network is back.
    return;
  }
  reconnect_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&ConnectionManager::StartSession, base::Unretained(this)));
}

void ConnectionManager::ScheduleReconcile(base::TimeDelta delay) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Restarting the timer on every event debounces bursts of notifications.
  reconcile_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&ConnectionManager::ReconcileConnectionStatus,
                     base::Unretained(this)));
}

void ConnectionManager::ReconcileConnectionStatus() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Only an established tunnel needs verifying. In every other state there is
  // either no tunnel or a transition that reports its own result.
  if (!network_available_ || !connection_api_ ||
      connection_state_ != mojom::VpnConnectionState::kConnected) {
    return;
  }
  // The result arrives through the Delegate methods. A healthy tunnel is
  // reported as OnConnectionEstablished(), which is ignored in kConnected. A
  // dead one is reported as OnConnectionTerminated(), which reconnects if the
  // user still wants the tunnel.
  connection_api_->CheckConnection();
}

}  // namespace brave_vpn::v2

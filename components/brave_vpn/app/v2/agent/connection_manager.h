/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/power_monitor/power_observer.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/app/v2/agent/connection_api.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "net/base/backoff_entry.h"
#include "net/base/network_change_notifier.h"

namespace brave_vpn::v2 {

// ConnectionManager owns the agent's VPN connection and tracks its state. There
// is one instance per agent process, shared by every connected browser through
// its BrowserHostImpl, so a call from any browser changes the state all of them
// observe. Must be created, used and destroyed on the main sequence.
//
// The manager separates the user's intent from the connection state. The
// connection state is the manager's single source of truth for what it is
// doing, and every event is handled according to it:
// - kDisconnected: no tunnel and nothing in progress;
// - kConnecting: connect request was sent to ConnectionApi; its result is
// pending;
// - kConnected: the tunnel is up;
// - kDisconnecting: the tunnel is going down, on request or on its own;
// - kWaitingRetry: an attempt failed or the tunnel dropped; waiting for the
// backoff timer, or for the network to come back;
// - kConnectFailed: gave up; the user's intent is cleared;
// - kInternalError: gave up because of a local problem that has nothing to do
// with the VPN server (ConnectionApi::FailureReason::kInternalError); such a
// failure is never retried. The user's intent is cleared.
//
// The intent is set by Connect() and cleared by Disconnect(). The manager
// clears it on its own only when the tunnel is lost and it is not going to
// restore it. What it does without being asked:
// - Retries: with auto-reconnect on, a failed attempt or a lost tunnel is
//   retried with a growing backoff, up to kMaxReconnectAttempts times. The
//   backoff starts over once a tunnel has stayed up for
//   kStableConnectionPeriod;
// - Timeouts: a connect or disconnect that ConnectionApi doesn't answer within
//   kTransitionTimeout is treated as failed, and the API instance is discarded;
// - Network changes: they never change the intent. While connected they trigger
//   a check of the tunnel. While waiting to retry they bring the retry
//   forward, and no retry is made while the network is down;
// - Suspend and resume: the manager's timers are stopped while the system
//   sleeps, so time asleep counts neither against a request in progress nor
//   towards a stable connection. After resume the tunnel is checked, and a
//   pending retry starts over with a fresh backoff.
//
// Observers are notified synchronously, so a state change caused by Connect(),
// Disconnect() or a ConnectionApi::Delegate callback may be reported before
// that call returns.
class ConnectionManager
    : public ConnectionApi::Delegate,
      public net::NetworkChangeNotifier::NetworkChangeObserver,
      public base::PowerSuspendObserver {
 public:
  // Observers must not call back into the manager from a notification: the
  // manager may be in the middle of handling the event that caused it.
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(
        mojom::VpnConnectionState state,
        std::optional<std::string> error_message) = 0;
    virtual void OnRegionSelectionChanged(
        const mojom::VpnRegionSelection& selection) = 0;
    virtual void OnAutoReconnectStateChanged(bool enabled) = 0;
    virtual void OnSmartProxyRoutingStateChanged(bool enabled) = 0;
    virtual void OnAllowLanTrafficStateChanged(bool available,
                                               bool enabled) = 0;
    virtual void OnLastHostnameChanged(const std::string& hostname) = 0;

   protected:
    ~Observer() override = default;
  };

  using ConnectionApiFactory =
      base::RepeatingCallback<std::unique_ptr<ConnectionApi>(
          ConnectionApi::Delegate*)>;

  using GetAllRegionsCallback =
      base::OnceCallback<void(std::vector<mojom::VpnRegionPtr>)>;

  explicit ConnectionManager(ConnectionApiFactory api_factory);
  ~ConnectionManager() override;

  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObservers() const;

  mojom::VpnConnectionState connection_state() const;
  const std::optional<std::string>& error_message() const;
  const mojom::VpnRegionSelection& region_selection() const;
  bool auto_reconnect_enabled() const;
  bool smart_proxy_routing_enabled() const;
  bool allow_lan_traffic_available() const;
  bool allow_lan_traffic_enabled() const;
  const std::string& last_hostname() const;

  // Starts connecting the VPN and begins a fresh retry sequence. Progress and
  // the result are reported through observer's OnConnectionStateChanged. If the
  // tunnel is still going down, connecting starts once that completes.
  virtual void Connect();
  // Disconnects the VPN and cancels any pending retry. Progress and the result
  // are reported through observer's OnConnectionStateChanged.
  virtual void Disconnect();
  // Clears a failure state and its error message, returning to kDisconnected
  // without connecting; the change is reported through observer's
  // OnConnectionStateChanged.
  virtual void ResetConnectionState();
  // Gets information about all available regions from an internal cache which
  // is fetched from the VPN provider. |callback| always runs asynchronously,
  // and runs with an empty list if they can't be fetched.
  virtual void GetAllRegions(GetAllRegionsCallback callback);
  // Selects the region by name and persists the choice. Once the region list
  // is available, the name is checked against it: if no region has that name,
  // the manager falls back to automatic selection. Any resulting change is
  // reported through observer's OnRegionSelectionChanged; nothing is reported
  // before the region list is available.
  virtual void SelectRegion(const std::string& name);
  // Returns to automatic selection and persists that. Any resulting change is
  // reported through observer's OnRegionSelectionChanged.
  virtual void SelectRegionAutomatically();
  // Sets the desired auto-reconnect state; this is a suggestion for the
  // agent and can be rejected; if the actual state gets changed, an update
  // will be dispatched via the corresponding observer callback.
  virtual void SetAutoReconnectState(bool enable);
  // Sets the desired Smart Proxy routing state; this is a suggestion for the
  // agent and can be rejected; if the actual state gets changed, an update
  // will be dispatched via the corresponding observer callback.
  virtual void SetSmartProxyRoutingState(bool enable);
  // Sets the desired LAN traffic allowance state; this is a suggestion for the
  // agent and can be rejected; if the actual state gets changed, an update
  // will be dispatched via the corresponding observer callback.
  virtual void SetAllowLanTrafficState(bool enable);

 protected:
  // Each records the new value and notifies observers. Does nothing if the
  // value didn't change.
  void UpdateConnectionState(mojom::VpnConnectionState state,
                             std::optional<std::string> error_message);
  void UpdateRegionSelection(mojom::VpnRegionSelectionPtr selection);
  void UpdateAutoReconnectState(bool enabled);
  void UpdateSmartProxyRoutingState(bool enabled);
  void UpdateAllowLanTrafficState(bool available, bool enabled);
  void UpdateLastHostname(std::string hostname);

 private:
  friend class ConnectionManagerSessionTest;

  // Quiet period after the last network change before checking the tunnel or
  // retrying.
  static constexpr base::TimeDelta kNetworkSettleDelay = base::Seconds(2);
  // Interfaces and routes take longer to come back after resume.
  static constexpr base::TimeDelta kResumeSettleDelay = base::Seconds(4);
  // Network changes this soon after our own tunnel came up or went down are
  // attributed to the tunnel itself. NetworkChangeNotifier holds a change back
  // for up to 2 seconds before reporting it, so this has to be well above that.
  static constexpr base::TimeDelta kTransitionGracePeriod = base::Seconds(5);
  // How long a new tunnel has to stay up before the retry sequence starts over.
  // A tunnel that keeps dropping sooner than this keeps backing off.
  static constexpr base::TimeDelta kStableConnectionPeriod = base::Seconds(10);
  // How long ConnectionApi may take to report the result of a connect or a
  // disconnect before the manager gives up on it.
  static constexpr base::TimeDelta kTransitionTimeout = base::Seconds(30);
  // Reconnect attempts made after a failed attempt or a lost tunnel before
  // giving up and reporting kConnectFailed.
  static constexpr int kMaxReconnectAttempts = 6;
  // Backoff policy for reconnect attempts after a failed connection or a lost
  // tunnel.
  static constexpr net::BackoffEntry::Policy kReconnectBackoffPolicy = {
      .num_errors_to_ignore = 0,
      .initial_delay_ms = 1000,
      .multiply_factor = 2.0,
      .jitter_factor = 0.2,
      .maximum_backoff_ms = 60 * 1000,
      .entry_lifetime_ms = -1,
      .always_use_initial_delay = false,
  };

  // ConnectionApi::Delegate:
  void OnConnectionEstablished() override;
  void OnConnectionTearingDown() override;
  void OnConnectionTerminated() override;
  void OnConnectionFailed(ConnectionApi::FailureReason reason) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // base::PowerSuspendObserver:
  void OnSuspend() override;
  void OnResume() override;

  // Creates the ConnectionApi on first use. The API reports to the delegate it
  // is given, must stop everything it is doing when destroyed, and must not
  // call the delegate from its destructor.
  ConnectionApi* GetOrCreateConnectionApi();

  // Moves to the connecting state and asks the API to connect.
  void StartSession();
  // Moves to |state| (must be either kConnecting or kDisconnecting) and starts
  // the watchdog that bounds how long the API may take to report the result.
  void BeginTransition(mojom::VpnConnectionState state);
  // Start the transition watchdog and the stable connection timer. OnResume()
  // uses them to restart the timers that OnSuspend() stopped.
  void StartTransitionWatchdog();
  void StartConnectionStableTimer();
  // True shortly after our own tunnel came up or went down, when network change
  // notifications are most likely caused by the tunnel itself.
  bool IsTransitionSettling() const;
  // Decides what follows the end of an attempt or of a tunnel: disconnected, a
  // new attempt, a retry, or failure. |end_state| is the state to settle in if
  // the tunnel isn't going to be restored: kDisconnected for a plain end of
  // the tunnel, kConnectFailed or kInternalError for an error. |retryable| is
  // false for errors that another attempt can't fix.
  void OnSessionFinished(mojom::VpnConnectionState end_state, bool retryable);
  void OnTransitionTimedOut();
  void OnConnectionStable();
  // Starts the retry timer, unless the network is down; OnNetworkChanged()
  // starts it once the network is back.
  void ArmRetryTimer(base::TimeDelta delay);
  void ScheduleReconcile(base::TimeDelta delay);
  void ReconcileConnectionStatus();

  SEQUENCE_CHECKER(sequence_checker_);
  base::ObserverList<Observer> observers_;
  ConnectionApiFactory connection_api_factory_;
  std::unique_ptr<ConnectionApi> connection_api_;
  mojom::VpnConnectionState connection_state_ =
      mojom::VpnConnectionState::kDisconnected;
  std::optional<std::string> error_message_;
  mojom::VpnRegionSelectionPtr region_selection_ =
      mojom::VpnRegionSelection::New(/*is_automatic=*/true, /*region=*/nullptr);
  bool auto_reconnect_enabled_ = false;
  bool smart_proxy_routing_enabled_ = false;
  bool allow_lan_traffic_available_ = false;
  bool allow_lan_traffic_enabled_ = false;
  std::string last_hostname_;
  bool network_available_;
  bool network_was_lost_ = false;
  bool desired_connected_ = false;
  bool connect_after_teardown_ = false;
  base::TimeTicks transition_settle_until_;
  base::OneShotTimer reconcile_timer_;
  base::OneShotTimer reconnect_timer_;
  base::OneShotTimer transition_watchdog_;
  base::OneShotTimer connection_stable_timer_;
  net::BackoffEntry reconnect_backoff_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_

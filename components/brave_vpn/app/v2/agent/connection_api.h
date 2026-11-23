/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_API_H_

#include "base/memory/raw_ref.h"
#include "base/sequence_checker.h"

namespace brave_vpn::v2 {

// ConnectionApi drives one VPN tunnel on behalf of its owner and reports what
// happens to the tunnel through Delegate. Implementations wrap a platform VPN
// stack. Must be created, used and destroyed on one sequence, and the delegate
// must outlive it.
//
// The owner relies on the rules below to tell which request a callback
// answers, so every implementation has to follow them:
// - Requests are asynchronous: Connect(), Disconnect() and CheckConnection()
//   return immediately and never call the delegate before they return; the
//   delegate is always called later, on the same sequence.
// - One request is in progress at a time, and a new request replaces it. The
//   replaced request gets no callbacks of its own: after Disconnect() aborts
//   a Connect(), the only result reported is the one for the Disconnect().
// - Every request is answered exactly once, within a bounded time. The
//   answers are listed at each method. Once a request has been answered or
//   the tunnel has ended, the delegate isn't called again until the next
//   request, except to report the loss of an established tunnel.
// - An established tunnel that is lost on its own is reported once, with
//   OnConnectionTerminated() or OnConnectionFailed(), optionally preceded by
//   OnConnectionTearingDown().
// - The owner never calls back into the API from a Delegate method, so
//   implementations don't have to support that.
// - Destroying the object stops whatever it is doing, takes the tunnel down
//   and never calls the delegate. The owner does this to recover from a
//   request that was never answered.
class ConnectionApi {
 public:
  // Describes why an attempt or a tunnel failed.
  enum class FailureReason {
    // The cause lies with the network or the VPN server. Another attempt may
    // succeed.
    kServerUnreachable,
    // A local problem that has nothing to do with the VPN server, such as being
    // denied access to the credentials in the system keychain. Another
    // attempt won't help until the problem is dealt with.
    kInternalError,
  };

  class Delegate {
   public:
    // The tunnel is up. Answers a Connect() that succeeded, and a
    // CheckConnection() that found the tunnel working.
    virtual void OnConnectionEstablished() = 0;
    // The tunnel has started going down. This is optional progress
    // information: it answers nothing, and is always followed by
    // OnConnectionTerminated() or OnConnectionFailed().
    virtual void OnConnectionTearingDown() = 0;
    // There is no tunnel anymore. Answers a Disconnect() and a
    // CheckConnection() that found the tunnel gone, and reports the loss of an
    // established tunnel when there is no particular reason to give.
    virtual void OnConnectionTerminated() = 0;
    // There is no tunnel, because of |reason|. Answers a Connect() that failed
    // and a Disconnect() whose teardown ran into an error, and reports the
    // loss of an established tunnel when the reason is known. Any partly built
    // tunnel has already been taken down.
    virtual void OnConnectionFailed(FailureReason reason) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  explicit ConnectionApi(Delegate* delegate);
  virtual ~ConnectionApi();

  ConnectionApi(const ConnectionApi&) = delete;
  ConnectionApi& operator=(const ConnectionApi&) = delete;

  // Brings the tunnel up. Answered with OnConnectionEstablished(), or with
  // OnConnectionFailed() if there is no tunnel in the end.
  void Connect();
  // Takes the tunnel down, or aborts a Connect() in progress. Answered with
  // OnConnectionTerminated(), also when there was no tunnel to begin with. A
  // teardown that runs into an error is answered with OnConnectionFailed(),
  // but the tunnel must be gone by then all the same.
  void Disconnect();
  // Verifies that an established tunnel still works. Answered with
  // OnConnectionEstablished() if it does and with OnConnectionTerminated() if
  // it doesn't; in the latter case the tunnel has been taken down. The owner
  // only calls this while it believes the tunnel is up.
  void CheckConnection();

 protected:
  Delegate& delegate() { return *delegate_; }

  virtual void ConnectImpl() = 0;
  virtual void DisconnectImpl() = 0;
  virtual void CheckConnectionImpl() = 0;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  const raw_ref<Delegate> delegate_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_API_H_

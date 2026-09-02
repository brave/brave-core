/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_registry.h"

#include <stdint.h>

#include <memory>

#include "base/auto_reset.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/process_handle.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_host_provider_impl.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_identity.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "components/named_mojo_ipc_server/fake_ipc_server.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
// A distinct pid per connection unless a test says otherwise, so the common
// case looks like separate browser processes.
base::ProcessId PidForConnection(mojo::ReceiverId connection) {
  return static_cast<base::ProcessId>(1000 + connection);
}
}  // namespace

class BrowserRegistryTest : public testing::Test {
 protected:
  BrowserRegistryTest()
      : identity_capture_callback_reset_(
            SetBrowserIdentityCaptureCallbackForTesting(base::BindRepeating(
                &BrowserRegistryTest::CaptureIdentityForConnection,
                base::Unretained(this)))) {
    registry_ = BrowserRegistry::CreateForTesting(
        std::make_unique<named_mojo_ipc_server::FakeIpcServer>(&server_state_));
  }

  scoped_refptr<BrowserIdentity> CaptureIdentityForConnection(
      const named_mojo_ipc_server::ConnectionInfo&) {
    if (identity_capture_fails_) {
      return nullptr;
    }
    return base::MakeRefCounted<FakeBrowserIdentity>(
        current_connection_pid_, base::BindLambdaForTesting([this]() {
          return identity_verification_result_;
        }),
        identity_is_same_process_);
  }

  brave_vpn::mojom::BrowserHostProvider* CallOnBrowserConnecting() {
    return registry_->OnBrowserConnecting(
        *server_state_.current_connection_info);
  }

  void CallAuthenticate(FakeBrowser& browser, uint32_t protocol_version) {
    registry_->Authenticate(protocol_version, browser.BindEndpoint(),
                            browser.BindHost(), browser.GetReplyCallback());
  }

  // Mojo's ReceiverSet hands out ids from a monotonic counter and never reuses
  // them, so a dropped connection's id can never come back.
  mojo::ReceiverId NextConnectionId() { return ++last_connection_id_; }

  // Points the fake server at |connection| and the fake factory at the pid that
  // connection belongs to, the way the real pair agree on both.
  void SetDispatchingConnection(mojo::ReceiverId connection) {
    server_state_.current_receiver = connection;
    server_state_.current_connection_info =
        std::make_unique<named_mojo_ipc_server::ConnectionInfo>();
    const auto it = connection_pids_.find(connection);
    current_connection_pid_ =
        it == connection_pids_.end() ? base::kNullProcessId : it->second;
  }

  // Drives the accept-time callback the real IPC server would make, which is
  // where the peer is captured. Nothing can authenticate without it.
  void SimulateConnect(mojo::ReceiverId connection, base::ProcessId pid) {
    connection_pids_[connection] = pid;
    SetDispatchingConnection(connection);
    EXPECT_TRUE(CallOnBrowserConnecting());
  }

  void StartAuthenticate(FakeBrowser& browser,
                         mojo::ReceiverId connection,
                         uint32_t protocol_version,
                         bool simulate_connect = true) {
    if (simulate_connect && !connection_pids_.contains(connection)) {
      SimulateConnect(connection, PidForConnection(connection));
    } else {
      SetDispatchingConnection(connection);
    }
    CallAuthenticate(browser, protocol_version);
  }

  mojom::BrowserAuthResult Authenticate(
      FakeBrowser& browser,
      mojo::ReceiverId connection,
      uint32_t protocol_version = mojom::kProtocolVersion) {
    StartAuthenticate(browser, connection, protocol_version,
                      /*simulate_connect=*/true);
    return browser.WaitForReply();
  }

  // Reports a dropped connection the way NamedMojoIpcServer does: the
  // disconnecting receiver is current while the handler runs.
  void DisconnectConnection(mojo::ReceiverId connection) {
    server_state_.current_receiver = connection;
    server_state_.disconnect_handler.Run();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  named_mojo_ipc_server::FakeIpcServer::TestState server_state_;
  mojo::ReceiverId last_connection_id_ = 0;
  base::flat_map<mojo::ReceiverId, base::ProcessId> connection_pids_;
  base::ProcessId current_connection_pid_ = base::kNullProcessId;
  bool identity_capture_fails_ = false;
  bool identity_is_same_process_ = true;
  BrowserIdentity::VerificationResult identity_verification_result_ =
      BrowserIdentity::VerificationResult::kAccepted;
  base::AutoReset<BrowserIdentityCaptureCallback>
      identity_capture_callback_reset_;
  std::unique_ptr<BrowserRegistry> registry_;
};

TEST_F(BrowserRegistryTest, StartsServingOnConstruction) {
  EXPECT_TRUE(server_state_.is_server_started);
  EXPECT_TRUE(server_state_.disconnect_handler);
}

TEST_F(BrowserRegistryTest, AcceptsBrowserAndBindsItsHost) {
  FakeBrowser browser;

  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(browser, NextConnectionId()));

  // The host pipe answering a round trip proves it was bound, not just that the
  // reply said so.
  browser.FlushHost();
  EXPECT_TRUE(browser.host_connected());
}

TEST_F(BrowserRegistryTest, RefusesProtocolVersionAboveTheAgentsOwn) {
  for (const bool simulate_connect : {false, true}) {
    FakeBrowser browser;
    StartAuthenticate(browser, NextConnectionId(), mojom::kProtocolVersion + 1,
                      simulate_connect);
    EXPECT_EQ(mojom::BrowserAuthResult::kVersionMismatch,
              browser.WaitForReply());
  }
}

TEST_F(BrowserRegistryTest, RefusesProtocolVersionBelowTheSupportedFloor) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;

  // Zero is below any floor the agent can declare.
  EXPECT_EQ(mojom::BrowserAuthResult::kVersionMismatch,
            Authenticate(browser, connection, /*protocol_version=*/0));

  // A refusal must leave nothing behind: the same connection can still
  // authenticate afterwards.
  FakeBrowser retry;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(retry, connection));
}

TEST_F(BrowserRegistryTest, RefusesSecondHostWhileOneIsBound) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, connection));

  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kHostAlreadyRequested,
            Authenticate(second, connection));
}

// The refusal has to cover a request that is outstanding, not only one that has
// already been granted.
TEST_F(BrowserRegistryTest, RefusesSecondHostWhileFirstIsStillInFlight) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  StartAuthenticate(first, connection, mojom::kProtocolVersion);
  ASSERT_FALSE(first.has_reply());

  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kHostAlreadyRequested,
            Authenticate(second, connection));

  // The pending request is unaffected by the refused one.
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted, first.WaitForReply());
}

TEST_F(BrowserRegistryTest, DroppingHostEndsSessionAndAllowsRebinding) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, connection));
  first.WatchEndpoint();

  first.DropHost();

  // Tearing the session down closes the endpoint the agent was calling back on.
  EXPECT_TRUE(first.WaitForEndpointClosed());

  // The connection itself is still up, so it may authenticate again.
  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, connection));
}

TEST_F(BrowserRegistryTest, DroppingHostWhileVerificationPendingEndsSession) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  StartAuthenticate(first, connection, mojom::kProtocolVersion);
  first.WatchEndpoint();

  first.DropHost();

  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted, first.WaitForReply());
  EXPECT_TRUE(first.WaitForEndpointClosed());

  // Neither the session nor the pending marker outlived the request.
  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, connection));
}

TEST_F(BrowserRegistryTest, DroppingEndpointEndsSessionAndAllowsRebinding) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, connection));
  first.WatchHost();

  first.DropEndpoint();

  EXPECT_TRUE(first.WaitForHostClosed());

  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, connection));
}

TEST_F(BrowserRegistryTest, ConnectionDropTearsDownItsSession) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(browser, connection));
  browser.WatchEndpoint();

  DisconnectConnection(connection);

  EXPECT_TRUE(browser.WaitForEndpointClosed());

  // Erasing during the disconnect handler must leave the registry able to serve
  // new connections. This cannot assert the dropped id itself is gone, since
  // ReceiverSet never reuses ids; the WaitForEndpointClosed() assertion is that
  // evidence.
  FakeBrowser next_browser;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(next_browser, NextConnectionId()));
}

// A connection that goes away mid-verification must still have its request
// answered, rather than leaving a mojo reply callback unrun.
TEST_F(BrowserRegistryTest, ConnectionDropDuringVerificationAnswersRequest) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;
  StartAuthenticate(browser, connection, mojom::kProtocolVersion);

  DisconnectConnection(connection);

  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive, browser.WaitForReply());

  // Nothing was left behind for the next connection.
  FakeBrowser next;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(next, NextConnectionId()));
}

// Shutdown while a request is being verified drops the request instead of
// answering it, which is what the weak pointer on the verification hop is for.
TEST_F(BrowserRegistryTest, DestroyedWhileVerificationPendingIsSafe) {
  FakeBrowser browser;
  StartAuthenticate(browser, NextConnectionId(), mojom::kProtocolVersion);
  browser.WatchEndpoint();

  registry_.reset();

  // The abandoned request takes the browser's endpoint down with it, which is
  // how the browser learns the agent will not answer.
  EXPECT_TRUE(browser.WaitForEndpointClosed());
  EXPECT_FALSE(browser.has_reply());
}

TEST_F(BrowserRegistryTest, KeepsOneSessionPerConnection) {
  const mojo::ReceiverId first_connection = NextConnectionId();
  const mojo::ReceiverId second_connection = NextConnectionId();
  FakeBrowser first;
  FakeBrowser second;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, first_connection));
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, second_connection));
  first.WatchEndpoint();
  second.WatchHost();

  DisconnectConnection(first_connection);
  ASSERT_TRUE(first.WaitForEndpointClosed());

  // Only the dropped connection's session goes away.
  second.FlushHost();
  EXPECT_TRUE(second.host_connected());
  EXPECT_FALSE(second.host_closed());
}

// A peer that cannot be pinned is refused before it ever reaches
// BindBrowserHost(): returning null from the accept callback is what refuses
// the connection.
TEST_F(BrowserRegistryTest, RefusesConnectionWhosePeerCannotBeCaptured) {
  identity_capture_fails_ = true;
  SetDispatchingConnection(NextConnectionId());
  EXPECT_FALSE(CallOnBrowserConnecting());
}

// A BindBrowserHost() on a connection the agent never accepted has no peer to
// verify. That is not a verdict about the caller, so it must be the retryable
// answer rather than a rejection.
TEST_F(BrowserRegistryTest, RefusesBrowserWithNoCaptureForItsConnection) {
  FakeBrowser browser;
  StartAuthenticate(browser, NextConnectionId(), mojom::kProtocolVersion,
                    /*simulate_connect=*/false);
  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive, browser.WaitForReply());
}

// The capture is only good for a bounded window, so a connection that sits
// silent past it can no longer authenticate.
TEST_F(BrowserRegistryTest, RefusesBrowserAfterItsCaptureExpires) {
  const mojo::ReceiverId connection = NextConnectionId();
  SimulateConnect(connection, PidForConnection(connection));

  task_environment_.FastForwardBy(base::Minutes(5));

  FakeBrowser browser;
  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive,
            Authenticate(browser, connection));
}

// A browser process holds one connection per profile and they arrive together,
// so every one of them has to resolve against the capture, not just whichever
// dispatches first.
TEST_F(BrowserRegistryTest, AcceptsSiblingConnectionsFromOneProcess) {
  constexpr base::ProcessId kSharedPid{12345};
  const mojo::ReceiverId first_connection = NextConnectionId();
  const mojo::ReceiverId second_connection = NextConnectionId();

  // Both connections are accepted before either authenticates.
  SimulateConnect(first_connection, kSharedPid);
  SimulateConnect(second_connection, kSharedPid);

  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, first_connection));

  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, second_connection));
}

// A capture belongs to the process it was taken for, so a connection reporting
// a different process must not resolve against it.
TEST_F(BrowserRegistryTest, DoesNotResolveAgainstAnotherProcessCapture) {
  const mojo::ReceiverId captured = NextConnectionId();
  SimulateConnect(captured, /*pid=*/1);

  // A connection that never went through the accept callback, reporting a pid
  // the registry holds no capture for.
  const mojo::ReceiverId uncaptured = NextConnectionId();
  connection_pids_[uncaptured] = 2;

  FakeBrowser browser;
  StartAuthenticate(browser, uncaptured, mojom::kProtocolVersion,
                    /*simulate_connect=*/false);
  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive, browser.WaitForReply());

  // The captured connection is unaffected.
  FakeBrowser other;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted, Authenticate(other, captured));
}

// A pid says which entry to look at; it does not say the entry describes the
// same process. When the two disagree the request must be refused rather than
// verified against a capture that was taken for someone else.
TEST_F(BrowserRegistryTest, RefusesBrowserWhosePidWasRecycled) {
  const mojo::ReceiverId connection = NextConnectionId();

  identity_is_same_process_ = false;
  SimulateConnect(connection, PidForConnection(connection));

  FakeBrowser browser;
  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive,
            Authenticate(browser, connection));
}

// The mismatched capture is dropped, so the process that really holds the pid
// now can connect and authenticate on a capture of its own.
TEST_F(BrowserRegistryTest, RecapturesAfterPidWasRecycled) {
  constexpr base::ProcessId kSharedPid{12345};

  identity_is_same_process_ = false;
  const mojo::ReceiverId stale_connection = NextConnectionId();
  SimulateConnect(stale_connection, kSharedPid);
  FakeBrowser stale;
  ASSERT_EQ(mojom::BrowserAuthResult::kInconclusive,
            Authenticate(stale, stale_connection));

  identity_is_same_process_ = true;
  const mojo::ReceiverId fresh_connection = NextConnectionId();
  SimulateConnect(fresh_connection, kSharedPid);
  FakeBrowser fresh;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(fresh, fresh_connection));
}

// Once a connection has authenticated, its identity belongs to the connection
// rather than to the accept-time capture, so re-binding a host still works long
// after that capture would have expired.
TEST_F(BrowserRegistryTest, AllowsRebindingAfterTheCaptureExpires) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, connection));

  first.DropHost();
  task_environment_.FastForwardBy(base::Minutes(5));

  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, connection));
}

// Dropping the connection drops its identity with it, so the new connection
// with the same receiver id cannot resolve against it and must authenticate on
// a fresh capture (which we intentionally leave out). This is not supposed to
// happen in practice, since ReceiverSet never reuses ids, but it is a safety
// check.
TEST_F(BrowserRegistryTest, ForgetsIdentityWhenTheConnectionGoesAway) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(first, connection, mojom::kProtocolVersion));

  DisconnectConnection(connection);
  task_environment_.FastForwardBy(base::Minutes(5));

  FakeBrowser second;
  StartAuthenticate(second, connection, mojom::kProtocolVersion,
                    /*simulate_connect=*/false);
  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive, second.WaitForReply());
}

// A verdict that the peer is not our browser reaches the browser as a rejection
// and leaves no session behind.
TEST_F(BrowserRegistryTest, RejectsBrowserThatFailsVerification) {
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kRejected;

  FakeBrowser browser;
  StartAuthenticate(browser, NextConnectionId(), mojom::kProtocolVersion);
  browser.WatchEndpoint();

  EXPECT_EQ(mojom::BrowserAuthResult::kRejected, browser.WaitForReply());

  // No session was created, so the handles the request carried are gone.
  EXPECT_TRUE(browser.WaitForEndpointClosed());
}

// Not being able to tell must never read as a rejection: it reaches the browser
// as the retryable answer, which is what keeps a browser whose image was
// replaced mid-update from going permanently unavailable.
TEST_F(BrowserRegistryTest, ReportsInconclusiveVerificationAsRetryable) {
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kInconclusive;

  FakeBrowser browser;
  StartAuthenticate(browser, NextConnectionId(), mojom::kProtocolVersion);
  browser.WatchEndpoint();

  EXPECT_EQ(mojom::BrowserAuthResult::kInconclusive, browser.WaitForReply());

  // No session was created, so the handles the request carried are gone.
  EXPECT_TRUE(browser.WaitForEndpointClosed());
}

// A refused verdict is about that one request: the connection stays usable, so
// the same peer can authenticate afterwards.
TEST_F(BrowserRegistryTest, AllowsAuthenticatingAfterFailedVerification) {
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kRejected;

  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BrowserAuthResult::kRejected,
            Authenticate(first, connection));

  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kAccepted;

  FakeBrowser second;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(second, connection));
}

// A capture that has expired must not hold a later connection from the same
// process to its old deadline.
TEST_F(BrowserRegistryTest, ReconnectingAfterExpiryGetsFreshCapture) {
  constexpr base::ProcessId kSharedPid{12345};

  const mojo::ReceiverId first_connection = NextConnectionId();
  SimulateConnect(first_connection, kSharedPid);

  task_environment_.FastForwardBy(base::Minutes(5));

  const mojo::ReceiverId second_connection = NextConnectionId();
  SimulateConnect(second_connection, kSharedPid);

  FakeBrowser browser;
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            Authenticate(browser, second_connection));
}

}  // namespace brave_vpn::v2

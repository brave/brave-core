/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_registry.h"

#include <stdint.h>

#include <memory>

#include "base/auto_reset.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
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
#include "mojo/public/cpp/platform/platform_handle.h"
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

  void CallInitialize(FakeBrowser& browser, uint32_t protocol_version) {
    registry_->InitializeBrowser(protocol_version, mojo::PlatformHandle(),
                                 browser.GetInitializeReplyCallback());
  }

  void CallBindBrowserHost(FakeBrowser& browser) {
    registry_->BindBrowserHost(browser.BindEndpoint(), browser.BindHost(),
                               browser.GetBindBrowserHostReplyCallback());
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

  void StartInitialize(FakeBrowser& browser,
                       mojo::ReceiverId connection,
                       uint32_t protocol_version,
                       bool simulate_connect = true) {
    if (simulate_connect && !connection_pids_.contains(connection)) {
      SimulateConnect(connection, PidForConnection(connection));
    } else {
      SetDispatchingConnection(connection);
    }
    CallInitialize(browser, protocol_version);
  }

  mojom::InitializeResult Initialize(
      FakeBrowser& browser,
      mojo::ReceiverId connection,
      uint32_t protocol_version = mojom::kProtocolVersion,
      bool simulate_connect = true) {
    StartInitialize(browser, connection, protocol_version, simulate_connect);
    const mojom::InitializeResult result = browser.WaitForInitializeReply();
    if (result == mojom::InitializeResult::kSuccess) {
      initialized_connections_.insert(connection);
    }
    return result;
  }

  // Brings |connection| to the point where it may ask for a host: accepted and
  // initialized, each at most once, since the registry allows one successful
  // Initialize() per connection. Initialize() carries the verification hop, so
  // this returns only once that verdict is in. A second attempt on the same
  // connection therefore only dispatches BindBrowserHost().
  void EnsureInitialized(FakeBrowser& browser, mojo::ReceiverId connection) {
    if (initialized_connections_.contains(connection)) {
      SetDispatchingConnection(connection);
      return;
    }
    ASSERT_EQ(mojom::InitializeResult::kSuccess,
              Initialize(browser, connection));
  }

  void StartBindBrowserHost(FakeBrowser& browser, mojo::ReceiverId connection) {
    EnsureInitialized(browser, connection);
    CallBindBrowserHost(browser);
  }

  mojom::BindBrowserHostResult BindBrowserHost(FakeBrowser& browser,
                                               mojo::ReceiverId connection) {
    StartBindBrowserHost(browser, connection);
    return browser.WaitForBindBrowserHostReply();
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
  base::flat_set<mojo::ReceiverId> initialized_connections_;
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

  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(browser, NextConnectionId()));

  // The host pipe answering a round trip proves it was bound, not just that the
  // reply said so.
  browser.FlushHost();
  EXPECT_TRUE(browser.host_connected());
}

TEST_F(BrowserRegistryTest, RefusesProtocolVersionAboveTheAgentsOwn) {
  for (const bool simulate_connect : {false, true}) {
    FakeBrowser browser;
    // The version is settled before the peer is resolved and verified, so an
    // unaccepted connection gets the same answer as an accepted one.
    EXPECT_EQ(mojom::InitializeResult::kVersionMismatch,
              Initialize(browser, NextConnectionId(),
                         mojom::kProtocolVersion + 1, simulate_connect));
  }
}

TEST_F(BrowserRegistryTest, RefusesProtocolVersionBelowTheSupportedFloor) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;

  // Zero is below any floor the agent can declare.
  EXPECT_EQ(mojom::InitializeResult::kVersionMismatch,
            Initialize(browser, connection, /*protocol_version=*/0));

  // A refusal must leave nothing behind: the same connection can still
  // initialize and authenticate afterwards.
  FakeBrowser retry;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(retry, connection));
}

// Nothing exists to bind against until Initialize() succeeds, and the browser
// has to be told which of the two calls it got wrong.
TEST_F(BrowserRegistryTest, RefusesHostBeforeInitialize) {
  const mojo::ReceiverId connection = NextConnectionId();
  SimulateConnect(connection, PidForConnection(connection));

  FakeBrowser browser;
  SetDispatchingConnection(connection);
  CallBindBrowserHost(browser);
  EXPECT_EQ(mojom::BindBrowserHostResult::kUninitialized,
            browser.WaitForBindBrowserHostReply());

  // The connection is still usable: a refusal is about the call, not the peer.
  FakeBrowser retry;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(retry, connection));
}

// Initialize() refused before verification starts leaves no entry behind, so a
// browser that ignores the result and asks for a host is told the connection is
// uninitialized rather than being bound against nothing.
TEST_F(BrowserRegistryTest, RefusesHostAfterFailedInitialize) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;
  ASSERT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(browser, connection, mojom::kProtocolVersion,
                       /*simulate_connect=*/false));

  CallBindBrowserHost(browser);
  EXPECT_EQ(mojom::BindBrowserHostResult::kUninitialized,
            browser.WaitForBindBrowserHostReply());
}

// One successful Initialize() per connection. Nothing about a connection's peer
// can change while it lives, so a second call is a bug or not our browser.
TEST_F(BrowserRegistryTest, RefusesSecondInitialize) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::InitializeResult::kSuccess, Initialize(first, connection));

  FakeBrowser second;
  SetDispatchingConnection(connection);
  CallInitialize(second, mojom::kProtocolVersion);
  EXPECT_EQ(mojom::InitializeResult::kInvalidRequest,
            second.WaitForInitializeReply());

  // The refusal does not disturb what the connection already has.
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, connection));
}

// The entry exists from the moment verification starts, which is what stops a
// peer asking again and making the agent verify it twice on one connection.
TEST_F(BrowserRegistryTest, RefusesSecondInitializeWhileVerifying) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  StartInitialize(first, connection, mojom::kProtocolVersion);
  ASSERT_FALSE(first.has_initialize_reply());

  FakeBrowser second;
  SetDispatchingConnection(connection);
  CallInitialize(second, mojom::kProtocolVersion);
  EXPECT_EQ(mojom::InitializeResult::kInvalidRequest,
            second.WaitForInitializeReply());

  // The first attempt is unaffected by the refused one.
  EXPECT_EQ(mojom::InitializeResult::kSuccess, first.WaitForInitializeReply());
}

// The gate on BindBrowserHost() is the verdict, not the existence of an entry:
// a connection whose verification is still in flight has an entry and must not
// get a host.
TEST_F(BrowserRegistryTest, RefusesHostWhileVerifying) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;
  StartInitialize(browser, connection, mojom::kProtocolVersion);
  ASSERT_FALSE(browser.has_initialize_reply());

  FakeBrowser early;
  SetDispatchingConnection(connection);
  CallBindBrowserHost(early);
  EXPECT_EQ(mojom::BindBrowserHostResult::kUninitialized,
            early.WaitForBindBrowserHostReply());

  // Once verified, the same connection binds normally.
  ASSERT_EQ(mojom::InitializeResult::kSuccess,
            browser.WaitForInitializeReply());
  FakeBrowser late;
  SetDispatchingConnection(connection);
  CallBindBrowserHost(late);
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            late.WaitForBindBrowserHostReply());
}

TEST_F(BrowserRegistryTest, RefusesSecondHostWhileOneIsBound) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, connection));

  FakeBrowser second;
  EXPECT_EQ(mojom::BindBrowserHostResult::kAlreadyBound,
            BindBrowserHost(second, connection));
}

// The gate is the bound session, not the reply having reached the browser: the
// host exists from the moment the first request is dispatched.
TEST_F(BrowserRegistryTest, RefusesSecondHostWhileFirstIsStillInFlight) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  StartBindBrowserHost(first, connection);
  ASSERT_FALSE(first.has_bind_browser_host_reply());

  FakeBrowser second;
  EXPECT_EQ(mojom::BindBrowserHostResult::kAlreadyBound,
            BindBrowserHost(second, connection));

  // The first request's reply still arrives, unaffected by the refused one.
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            first.WaitForBindBrowserHostReply());
}

TEST_F(BrowserRegistryTest, DroppingHostEndsSessionAndAllowsRebinding) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, connection));
  first.WatchEndpoint();

  first.DropHost();

  // Tearing the session down closes the endpoint the agent was calling back on.
  EXPECT_TRUE(first.WaitForEndpointClosed());

  // The connection itself is still up, so it may authenticate again.
  FakeBrowser second;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(second, connection));
}

// The host is created before its reply is posted, so a pipe that drops in
// between must still tear the session down.
TEST_F(BrowserRegistryTest, DroppingHostBeforeReplyEndsSession) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  StartBindBrowserHost(first, connection);
  first.WatchEndpoint();

  first.DropHost();

  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            first.WaitForBindBrowserHostReply());
  EXPECT_TRUE(first.WaitForEndpointClosed());

  // The session did not outlive the pipe, so the connection can bind again.
  FakeBrowser second;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(second, connection));
}

TEST_F(BrowserRegistryTest, DroppingEndpointEndsSessionAndAllowsRebinding) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, connection));
  first.WatchHost();

  first.DropEndpoint();

  EXPECT_TRUE(first.WaitForHostClosed());

  FakeBrowser second;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(second, connection));
}

TEST_F(BrowserRegistryTest, ConnectionDropTearsDownItsSession) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(browser, connection));
  browser.WatchEndpoint();

  DisconnectConnection(connection);

  EXPECT_TRUE(browser.WaitForEndpointClosed());

  // Erasing during the disconnect handler must leave the registry able to serve
  // new connections. This cannot assert the dropped id itself is gone, since
  // ReceiverSet never reuses ids; the WaitForEndpointClosed() assertion is that
  // evidence.
  FakeBrowser next_browser;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(next_browser, NextConnectionId()));
}

// A connection that goes away mid-verification must still have its Initialize()
// answered, rather than leaving a mojo reply callback unrun.
TEST_F(BrowserRegistryTest, ConnectionDropDuringVerificationAnswersRequest) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser browser;
  StartInitialize(browser, connection, mojom::kProtocolVersion);
  ASSERT_FALSE(browser.has_initialize_reply());

  DisconnectConnection(connection);

  EXPECT_EQ(mojom::InitializeResult::kInconclusive,
            browser.WaitForInitializeReply());

  // Nothing was left behind for the next connection.
  FakeBrowser next;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(next, NextConnectionId()));
}

// Shutdown while a peer is being verified drops the request instead of
// answering it, which is what the weak pointer on the verification hop is for.
TEST_F(BrowserRegistryTest, DestroyedWhileVerificationPendingIsSafe) {
  FakeBrowser browser;
  StartInitialize(browser, NextConnectionId(), mojom::kProtocolVersion);

  registry_.reset();

  // The verification reply is already queued, so the sequence has to run for
  // the weak pointer to get its chance to cancel it. Without this the
  // assertion below would hold even if it did not.
  task_environment_.FastForwardUntilNoTasksRemain();

  EXPECT_FALSE(browser.has_initialize_reply());
}

// Shutdown tears down live sessions, which is how a browser learns the agent is
// gone rather than being left with a host that answers nothing.
TEST_F(BrowserRegistryTest, DestroyedWithLiveSessionClosesIt) {
  FakeBrowser browser;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(browser, NextConnectionId()));
  browser.WatchEndpoint();

  registry_.reset();

  EXPECT_TRUE(browser.WaitForEndpointClosed());
}

TEST_F(BrowserRegistryTest, KeepsOneSessionPerConnection) {
  const mojo::ReceiverId first_connection = NextConnectionId();
  const mojo::ReceiverId second_connection = NextConnectionId();
  FakeBrowser first;
  FakeBrowser second;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, first_connection));
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(second, second_connection));
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

// An Initialize() on a connection the agent never accepted has no peer to
// verify. That is not a verdict about the caller, so it must be the retryable
// answer rather than a rejection.
TEST_F(BrowserRegistryTest, RefusesBrowserWithNoCaptureForItsConnection) {
  FakeBrowser browser;
  EXPECT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(browser, NextConnectionId(), mojom::kProtocolVersion,
                       /*simulate_connect=*/false));
}

// The dispatch-time capture is how the peer's pid is read, so a capture that
// fails there leaves nothing to resolve against. This is the branch a broken
// platform implementation hits first.
TEST_F(BrowserRegistryTest, RefusesBrowserWhosePeerCannotBeCapturedAtDispatch) {
  const mojo::ReceiverId connection = NextConnectionId();
  SimulateConnect(connection, PidForConnection(connection));

  identity_capture_fails_ = true;
  FakeBrowser browser;
  EXPECT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(browser, connection));
}

// The capture is only good for a bounded window, so a connection that sits
// silent past it can no longer initialize.
TEST_F(BrowserRegistryTest, RefusesBrowserAfterItsCaptureExpires) {
  const mojo::ReceiverId connection = NextConnectionId();
  SimulateConnect(connection, PidForConnection(connection));

  task_environment_.FastForwardBy(base::Minutes(5));

  FakeBrowser browser;
  EXPECT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(browser, connection));
}

// A browser process holds one connection per profile and they arrive together,
// so every one of them has to resolve against the capture, not just whichever
// dispatches first. Both verifications are also in flight at once, which is a
// state the registry only entered once verification moved into Initialize().
TEST_F(BrowserRegistryTest, AcceptsSiblingConnectionsFromOneProcess) {
  constexpr base::ProcessId kSharedPid{12345};
  const mojo::ReceiverId first_connection = NextConnectionId();
  const mojo::ReceiverId second_connection = NextConnectionId();

  // Both connections are accepted before either initializes.
  SimulateConnect(first_connection, kSharedPid);
  SimulateConnect(second_connection, kSharedPid);

  FakeBrowser first;
  FakeBrowser second;
  StartInitialize(first, first_connection, mojom::kProtocolVersion);
  StartInitialize(second, second_connection, mojom::kProtocolVersion);
  ASSERT_FALSE(first.has_initialize_reply());
  ASSERT_FALSE(second.has_initialize_reply());

  EXPECT_EQ(mojom::InitializeResult::kSuccess, first.WaitForInitializeReply());
  EXPECT_EQ(mojom::InitializeResult::kSuccess, second.WaitForInitializeReply());
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
  EXPECT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(browser, uncaptured, mojom::kProtocolVersion,
                       /*simulate_connect=*/false));

  // The captured connection is unaffected.
  FakeBrowser other;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(other, captured));
}

// A pid says which entry to look at; it does not say the entry describes the
// same process. When the two disagree the request must be refused rather than
// verified against a capture that was taken for someone else.
TEST_F(BrowserRegistryTest, RefusesBrowserWhosePidWasRecycled) {
  const mojo::ReceiverId connection = NextConnectionId();

  identity_is_same_process_ = false;
  SimulateConnect(connection, PidForConnection(connection));

  FakeBrowser browser;
  EXPECT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(browser, connection));
}

// The mismatched capture is dropped, so the process that really holds the pid
// now can connect and authenticate on a capture of its own.
TEST_F(BrowserRegistryTest, RecapturesAfterPidWasRecycled) {
  constexpr base::ProcessId kSharedPid{12345};

  identity_is_same_process_ = false;
  const mojo::ReceiverId stale_connection = NextConnectionId();
  SimulateConnect(stale_connection, kSharedPid);
  FakeBrowser stale;
  ASSERT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(stale, stale_connection));

  identity_is_same_process_ = true;
  const mojo::ReceiverId fresh_connection = NextConnectionId();
  SimulateConnect(fresh_connection, kSharedPid);
  FakeBrowser fresh;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(fresh, fresh_connection));
}

// Once a connection has authenticated, its identity belongs to the connection
// rather than to the accept-time capture, so re-binding a host still works long
// after that capture would have expired.
TEST_F(BrowserRegistryTest, AllowsRebindingAfterTheCaptureExpires) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, connection));

  first.DropHost();
  task_environment_.FastForwardBy(base::Minutes(5));

  // The verdict belongs to the connection, so a rebind must not consult it
  // again: this would refuse if it did.
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kRejected;

  FakeBrowser second;
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(second, connection));
}

// Dropping the connection drops its identity with it, so the new connection
// with the same receiver id cannot resolve against it and must initialize on a
// fresh capture (which we intentionally leave out). This is not supposed to
// happen in practice, since ReceiverSet never reuses ids, but it is a safety
// check.
TEST_F(BrowserRegistryTest, ForgetsIdentityWhenTheConnectionGoesAway) {
  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(first, connection));

  DisconnectConnection(connection);
  task_environment_.FastForwardBy(base::Minutes(5));

  FakeBrowser second;
  EXPECT_EQ(mojom::InitializeResult::kNotIdentified,
            Initialize(second, connection, mojom::kProtocolVersion,
                       /*simulate_connect=*/false));
}

// A verdict that the peer is not our browser reaches the browser as a rejection
// on Initialize(), before it has handed over any session handles.
TEST_F(BrowserRegistryTest, RejectsBrowserThatFailsVerification) {
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kRejected;

  FakeBrowser browser;
  EXPECT_EQ(mojom::InitializeResult::kRejected,
            Initialize(browser, NextConnectionId()));
}

// Not being able to tell must never read as a rejection: it reaches the browser
// as the retryable answer, which is what keeps a browser whose image was
// replaced mid-update from going permanently unavailable.
TEST_F(BrowserRegistryTest, ReportsInconclusiveVerificationAsRetryable) {
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kInconclusive;

  FakeBrowser browser;
  EXPECT_EQ(mojom::InitializeResult::kInconclusive,
            Initialize(browser, NextConnectionId()));
}

// A verification refusal spends the connection. The entry stays behind on
// purpose: without it, a peer could re-call Initialize() and make the agent
// verify it again and again on one connection.
TEST_F(BrowserRegistryTest, FailedVerificationSpendsTheConnection) {
  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kRejected;

  const mojo::ReceiverId connection = NextConnectionId();
  FakeBrowser first;
  ASSERT_EQ(mojom::InitializeResult::kRejected, Initialize(first, connection));

  identity_verification_result_ =
      BrowserIdentity::VerificationResult::kAccepted;

  // Asking again on the same connection is refused whatever the verdict would
  // now be, and no host can be bound either.
  FakeBrowser second;
  SetDispatchingConnection(connection);
  CallInitialize(second, mojom::kProtocolVersion);
  EXPECT_EQ(mojom::InitializeResult::kInvalidRequest,
            second.WaitForInitializeReply());

  FakeBrowser third;
  SetDispatchingConnection(connection);
  CallBindBrowserHost(third);
  EXPECT_EQ(mojom::BindBrowserHostResult::kUninitialized,
            third.WaitForBindBrowserHostReply());
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
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            BindBrowserHost(browser, second_connection));
}

}  // namespace brave_vpn::v2

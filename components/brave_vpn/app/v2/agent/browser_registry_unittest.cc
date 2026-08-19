/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_registry.h"

#include <stdint.h>

#include <memory>

#include "base/test/task_environment.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_host_provider_impl.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "components/named_mojo_ipc_server/fake_ipc_server.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {

class BrowserRegistryTest : public testing::Test {
 protected:
  BrowserRegistryTest()
      : registry_(BrowserRegistry::CreateForTesting(
            std::make_unique<named_mojo_ipc_server::FakeIpcServer>(
                &server_state_))) {}

  // Mojo's ReceiverSet hands out ids from a monotonic counter and never reuses
  // them, so a dropped connection's id can never come back.
  mojo::ReceiverId NextConnectionId() { return ++last_connection_id_; }

  void StartAuthenticate(FakeBrowser& browser,
                         mojo::ReceiverId connection,
                         uint32_t protocol_version) {
    server_state_.current_receiver = connection;
    static_cast<BrowserHostProviderImpl::Delegate&>(*registry_)
        .Authenticate(protocol_version, browser.BindEndpoint(),
                      browser.BindHost(), browser.GetReplyCallback());
  }

  mojom::BrowserAuthResult Authenticate(
      FakeBrowser& browser,
      mojo::ReceiverId connection,
      uint32_t protocol_version = mojom::kProtocolVersion) {
    StartAuthenticate(browser, connection, protocol_version);
    return browser.WaitForReply();
  }

  // Reports a dropped connection the way NamedMojoIpcServer does: the
  // disconnecting receiver is current while the handler runs.
  void DisconnectConnection(mojo::ReceiverId connection) {
    server_state_.current_receiver = connection;
    server_state_.disconnect_handler.Run();
  }

  base::test::TaskEnvironment task_environment_;
  named_mojo_ipc_server::FakeIpcServer::TestState server_state_;
  std::unique_ptr<BrowserRegistry> registry_;
  mojo::ReceiverId last_connection_id_ = 0;
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
  FakeBrowser browser;

  EXPECT_EQ(
      mojom::BrowserAuthResult::kVersionMismatch,
      Authenticate(browser, NextConnectionId(), mojom::kProtocolVersion + 1));
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

  EXPECT_EQ(mojom::BrowserAuthResult::kRejected, browser.WaitForReply());

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

}  // namespace brave_vpn::v2

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_host_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_endpoint.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_manager.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// Bounded barrier for asserting that something did not happen: a round trip on
// a throwaway pipe bound to this sequence completes only after the work already
// queued on the sequence has run. Needed when neither session pipe survives to
// be flushed directly.
void FlushSequence() {
  FakeBrowserEndpoint endpoint_impl;
  mojo::Remote<mojom::BrowserEndpoint> remote;
  mojo::Receiver<mojom::BrowserEndpoint> receiver(
      &endpoint_impl, remote.BindNewPipeAndPassReceiver());
  remote.FlushForTesting();
}

}  // namespace

class BrowserHostImplTest : public testing::Test {
 protected:
  void CreateHost() { CreateHost(disconnected_.GetCallback()); }
  void CreateHost(base::OnceClosure disconnect_handler) {
    host_ = std::make_unique<BrowserHostImpl>(
        connection_manager_, browser_.BindEndpoint(), browser_.BindHost(),
        std::move(disconnect_handler));
    ASSERT_TRUE(host_);
  }

  // Runs until the endpoint has received |count| hostname updates. Messages
  // on one pipe arrive in order, so every update sent before that hostname
  // has arrived too.
  [[nodiscard]] bool WaitForHostnameUpdates(size_t count) {
    return base::test::RunUntil([&] {
      return browser_.endpoint().last_hostname_updates().size() >= count;
    });
  }

  base::test::TaskEnvironment task_environment_;
  FakeConnectionManager connection_manager_;
  FakeBrowser browser_;
  std::unique_ptr<BrowserHostImpl> host_;
  base::test::TestFuture<void> disconnected_;
};

TEST_F(BrowserHostImplTest, StaysAliveWhileBothPipesAreOpen) {
  CreateHost();
  browser_.FlushHost();
  EXPECT_FALSE(disconnected_.IsReady());
}

TEST_F(BrowserHostImplTest, DroppingHostReportsDisconnect) {
  CreateHost();
  browser_.DropHost();
  EXPECT_TRUE(disconnected_.Wait());
}

// A browser that drops only its endpoint can no longer be called back, so the
// session must end even though the BrowserHost pipe is still open.
TEST_F(BrowserHostImplTest, DroppingEndpointReportsDisconnect) {
  CreateHost();
  browser_.DropEndpoint();
  EXPECT_TRUE(disconnected_.Wait());
}

// Both pipes closing must still report exactly once, since the report is what
// tears the session down. The handler here deliberately keeps the host alive,
// so the surviving pipe's handler really does run and find the report consumed.
TEST_F(BrowserHostImplTest, ReportsDisconnectOnceWhenBothPipesClose) {
  int report_count = 0;
  base::RunLoop first_report;
  CreateHost(base::BindLambdaForTesting([&] {
    ++report_count;
    first_report.Quit();
  }));

  browser_.DropEndpoint();
  first_report.Run();
  ASSERT_EQ(1, report_count);

  browser_.DropHost();
  FlushSequence();

  EXPECT_EQ(1, report_count);
}

// The registry destroys sessions itself when a connection goes away, and must
// not get a report back for a host it has already deleted.
TEST_F(BrowserHostImplTest, DestructionDoesNotReportDisconnect) {
  CreateHost();
  browser_.WatchHost();

  host_.reset();

  // The browser observing its own end close proves the teardown has been
  // processed, so the report either happened by now or never will.
  EXPECT_TRUE(browser_.WaitForHostClosed());
  EXPECT_FALSE(disconnected_.IsReady());
}

// A destroyed host must stop observing the shared manager, or the manager
// would go on notifying a destroyed observer.
TEST_F(BrowserHostImplTest, StopsObservingManagerOnDestruction) {
  CreateHost();
  ASSERT_TRUE(connection_manager_.HasObservers());
  host_.reset();
  EXPECT_FALSE(connection_manager_.HasObservers());
}

// The browser learns the current state without asking for it, one update of
// each kind. The manager is shared, so a browser that binds late must get what
// other browsers already caused: non-default values prove the push reads the
// manager rather than sending defaults.
TEST_F(BrowserHostImplTest, SendsCurrentStateOnCreation) {
  connection_manager_.UpdateConnectionState(
      mojom::VpnConnectionState::kConnectFailed, "Server unreachable");
  connection_manager_.UpdateSelectedRegion(mojom::VpnRegion::New("Europe"));
  connection_manager_.UpdateAutoReconnectState(true);
  connection_manager_.UpdateSmartProxyRoutingState(true);
  connection_manager_.UpdateAllowLanTrafficState(true, true);
  connection_manager_.UpdateLastHostname("eu-1.example.com");

  CreateHost();
  ASSERT_TRUE(WaitForHostnameUpdates(1));
  // Rules out a duplicate push queued right behind the first one.
  FlushSequence();

  const FakeBrowserEndpoint& endpoint = browser_.endpoint();

  const auto& states = endpoint.connection_state_updates();
  ASSERT_EQ(1u, states.size());
  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed, states[0].state);
  EXPECT_EQ("Server unreachable", states[0].error_message);

  const auto& regions = endpoint.selected_region_updates();
  ASSERT_EQ(1u, regions.size());
  EXPECT_EQ("Europe", regions[0]->name);

  EXPECT_EQ(std::vector<bool>{true}, endpoint.auto_reconnect_state_updates());
  EXPECT_EQ(std::vector<bool>{true},
            endpoint.smart_proxy_routing_state_updates());

  const auto& lan = endpoint.allow_lan_traffic_state_updates();
  ASSERT_EQ(1u, lan.size());
  EXPECT_TRUE(lan[0].available);
  EXPECT_TRUE(lan[0].enabled);

  EXPECT_EQ(std::vector<std::string>{"eu-1.example.com"},
            endpoint.last_hostname_updates());
}

// Every kind of change reaches the browser, and connection state changes keep
// their order.
TEST_F(BrowserHostImplTest, RelaysStateChangesToEndpoint) {
  CreateHost();
  ASSERT_TRUE(WaitForHostnameUpdates(1));

  connection_manager_.UpdateConnectionState(
      mojom::VpnConnectionState::kConnecting, std::nullopt);
  connection_manager_.UpdateConnectionState(
      mojom::VpnConnectionState::kConnectFailed, "Server unreachable");
  connection_manager_.UpdateSelectedRegion(mojom::VpnRegion::New("Asia"));
  connection_manager_.UpdateAutoReconnectState(true);
  connection_manager_.UpdateSmartProxyRoutingState(true);
  connection_manager_.UpdateAllowLanTrafficState(true, false);
  connection_manager_.UpdateLastHostname("as-1.example.com");
  ASSERT_TRUE(WaitForHostnameUpdates(2));

  // Index 0 of each list is the initial push.
  const FakeBrowserEndpoint& endpoint = browser_.endpoint();

  const auto& states = endpoint.connection_state_updates();
  ASSERT_EQ(3u, states.size());
  EXPECT_EQ(mojom::VpnConnectionState::kConnecting, states[1].state);
  EXPECT_EQ(std::nullopt, states[1].error_message);
  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed, states[2].state);
  EXPECT_EQ("Server unreachable", states[2].error_message);

  const auto& regions = endpoint.selected_region_updates();
  ASSERT_EQ(2u, regions.size());
  EXPECT_EQ("Asia", regions[1]->name);

  EXPECT_EQ((std::vector<bool>{false, true}),
            endpoint.auto_reconnect_state_updates());
  EXPECT_EQ((std::vector<bool>{false, true}),
            endpoint.smart_proxy_routing_state_updates());

  const auto& lan = endpoint.allow_lan_traffic_state_updates();
  ASSERT_EQ(2u, lan.size());
  EXPECT_TRUE(lan[1].available);
  EXPECT_FALSE(lan[1].enabled);

  ASSERT_EQ(2u, endpoint.last_hostname_updates().size());
  EXPECT_EQ("as-1.example.com", endpoint.last_hostname_updates()[1]);
}

TEST_F(BrowserHostImplTest, ForwardsCallsToConnectionManager) {
  CreateHost();

  browser_.host()->Connect();
  browser_.host()->Disconnect();
  browser_.host()->ResetConnectionState();
  browser_.host()->SetSelectedRegion(mojom::VpnRegion::New("Europe"));
  browser_.host()->SetAutoReconnectState(true);
  browser_.host()->SetSmartProxyRoutingState(true);
  // False rather than true: proves the request was recorded, not just that the
  // optional happens to hold a truthy value.
  browser_.host()->SetAllowLanTrafficState(false);
  browser_.FlushHost();

  EXPECT_EQ(1, connection_manager_.connect_calls());
  EXPECT_EQ(1, connection_manager_.disconnect_calls());
  EXPECT_EQ(1, connection_manager_.reset_connection_state_calls());
  ASSERT_TRUE(connection_manager_.last_selected_region());
  EXPECT_EQ("Europe", connection_manager_.last_selected_region()->name);
  EXPECT_EQ(true, connection_manager_.last_auto_reconnect_request());
  EXPECT_EQ(true, connection_manager_.last_smart_proxy_routing_request());
  EXPECT_EQ(false, connection_manager_.last_allow_lan_traffic_request());
}

TEST_F(BrowserHostImplTest, ReturnsRegionsFromConnectionManager) {
  std::vector<mojom::VpnRegionPtr> regions;
  regions.push_back(mojom::VpnRegion::New("Europe"));
  regions.push_back(mojom::VpnRegion::New("Asia"));
  connection_manager_.set_regions(std::move(regions));
  CreateHost();

  base::test::TestFuture<std::vector<mojom::VpnRegionPtr>> future;
  browser_.host()->GetAllRegions(future.GetCallback());

  std::vector<mojom::VpnRegionPtr> result = future.Take();
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("Europe", result[0]->name);
  EXPECT_EQ("Asia", result[1]->name);
}

}  // namespace brave_vpn::v2

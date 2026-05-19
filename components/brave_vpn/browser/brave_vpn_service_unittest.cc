/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

namespace {

// Concrete test subclass providing stub implementations for BraveVpnService's
// pure virtual methods. Allows exercising the base-class behavior (observer
// management, notification dispatch, shutdown, mojo binding) in isolation.
class TestBraveVpnService : public BraveVpnService {
 public:
  TestBraveVpnService() = default;
  ~TestBraveVpnService() override = default;

  // BraveVpnService:
  bool IsBraveVPNEnabled() const override { return true; }
  bool IsPurchased() const override { return true; }
  void ReloadPurchasedState() override {}
  std::string GetCurrentEnvironment() const override { return "production"; }

#if !BUILDFLAG(IS_ANDROID)
  bool IsConnected() const override { return false; }
  void ToggleConnection() override {}
  mojom::ConnectionState GetConnectionState() const override {
    return mojom::ConnectionState::DISCONNECTED;
  }
  void RecordWidgetUsageMetrics(bool new_usage) override {}
#else
  void GetTimezonesForRegions(ResponseCallback callback) override {}
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region,
                             const std::string& region_precision) override {}
  void GetProfileCredentials(ResponseCallback callback,
                             const std::string& subscriber_credential,
                             const std::string& hostname) override {}
  void GetWireguardProfileCredentials(ResponseCallback callback,
                                      const std::string& subscriber_credential,
                                      const std::string& public_key,
                                      const std::string& hostname) override {}
  void VerifyCredentials(ResponseCallback callback,
                         const std::string& hostname,
                         const std::string& client_id,
                         const std::string& subscriber_credential,
                         const std::string& api_auth_token) override {}
  void InvalidateCredentials(ResponseCallback callback,
                             const std::string& hostname,
                             const std::string& client_id,
                             const std::string& subscriber_credential,
                             const std::string& api_auth_token) override {}
  void VerifyPurchaseToken(ResponseCallback callback,
                           const std::string& purchase_token,
                           const std::string& product_id,
                           const std::string& product_type,
                           const std::string& bundle_id) override {}
  void GetSubscriberCredential(ResponseCallback callback,
                               const std::string& product_type,
                               const std::string& product_id,
                               const std::string& validation_method,
                               const std::string& purchase_token,
                               const std::string& bundle_id) override {}
  void GetSubscriberCredentialV12(ResponseCallback callback) override {}
  void RecordAllMetrics() override {}
  void RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                  int64_t session_end_time_ms) override {}
#endif

#if !BUILDFLAG(IS_ANDROID)
  void SetConnectionStateForTesting(mojom::ConnectionState state) override {}
  void SetPurchasedStateForTesting(const std::string& env,
                                   mojom::PurchasedState state) override {}
#endif

  // Re-expose the protected notification helpers and Shutdown for tests.
  using BraveVpnService::NotifyPurchasedStateChanged;
  using BraveVpnService::Shutdown;
#if !BUILDFLAG(IS_ANDROID)
  using BraveVpnService::NotifyConnectionStateChanged;
  using BraveVpnService::NotifySelectedRegionChanged;
  using BraveVpnService::NotifySmartProxyRoutingStateChanged;
#endif

  // mojom::ServiceHandler
  void GetPurchasedState(GetPurchasedStateCallback callback) override {}
  void LoadPurchasedState(const std::string& domain) override {}
  void GetAllRegions(GetAllRegionsCallback callback) override {}
#if !BUILDFLAG(IS_ANDROID)
  void GetConnectionState(GetConnectionStateCallback callback) override {}
  void Connect() override {}
  void Disconnect() override {}
  void GetSelectedRegion(GetSelectedRegionCallback callback) override {}
  void SetSelectedRegion(mojom::RegionPtr region) override {}
  void ClearSelectedRegion() override {}
  void GetProductUrls(GetProductUrlsCallback callback) override {}
  void CreateSupportTicket(const std::string& email,
                           const std::string& subject,
                           const std::string& body,
                           CreateSupportTicketCallback callback) override {}
  void GetSupportData(GetSupportDataCallback callback) override {}
  void ResetConnectionState() override {}
  void EnableOnDemand(bool enable) override {}
  void GetOnDemandState(GetOnDemandStateCallback callback) override {}
  void EnableSmartProxyRouting(bool enable) override {}
  void GetSmartProxyRoutingState(
      GetSmartProxyRoutingStateCallback callback) override {}
#else   // !BUILDFLAG(IS_ANDROID)
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override {}
#endif  // !BUILDFLAG(IS_ANDROID)
};

// Test observer that records all callbacks delivered over the mojo pipe and
// optionally fires a one-shot closure for synchronization with RunLoop.
class TestServiceObserver : public mojom::ServiceObserver {
 public:
  struct PurchasedStateCall {
    mojom::PurchasedState state;
    std::optional<std::string> description;
  };

  TestServiceObserver() = default;
  ~TestServiceObserver() override = default;

  mojo::PendingRemote<mojom::ServiceObserver> BindAndGetRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void ResetReceiver() { receiver_.reset(); }

  void set_disconnect_handler(base::OnceClosure handler) {
    receiver_.set_disconnect_handler(std::move(handler));
  }

  // mojom::ServiceObserver:
  void OnPurchasedStateChanged(
      mojom::PurchasedState state,
      const std::optional<std::string>& description) override {
    purchased_state_calls_.push_back({state, description});
    MaybeQuit(purchased_state_quit_);
  }

#if !BUILDFLAG(IS_ANDROID)
  void OnConnectionStateChanged(mojom::ConnectionState state) override {
    connection_state_calls_.push_back(state);
    MaybeQuit(connection_state_quit_);
  }

  void OnSelectedRegionChanged(mojom::RegionPtr region) override {
    selected_region_calls_.push_back(std::move(region));
    MaybeQuit(selected_region_quit_);
  }

  void OnSmartProxyRoutingStateChanged(bool enabled) override {
    smart_proxy_calls_.push_back(enabled);
    MaybeQuit(smart_proxy_quit_);
  }
#endif

  const std::vector<PurchasedStateCall>& purchased_state_calls() const {
    return purchased_state_calls_;
  }

  void WaitForNextPurchasedStateCall() {
    base::RunLoop loop;
    purchased_state_quit_ = loop.QuitClosure();
    loop.Run();
  }

#if !BUILDFLAG(IS_ANDROID)
  const std::vector<mojom::ConnectionState>& connection_state_calls() const {
    return connection_state_calls_;
  }
  const std::vector<mojom::RegionPtr>& selected_region_calls() const {
    return selected_region_calls_;
  }
  const std::vector<bool>& smart_proxy_calls() const {
    return smart_proxy_calls_;
  }

  void WaitForNextConnectionStateCall() {
    base::RunLoop loop;
    connection_state_quit_ = loop.QuitClosure();
    loop.Run();
  }
  void WaitForNextSelectedRegionCall() {
    base::RunLoop loop;
    selected_region_quit_ = loop.QuitClosure();
    loop.Run();
  }
  void WaitForNextSmartProxyCall() {
    base::RunLoop loop;
    smart_proxy_quit_ = loop.QuitClosure();
    loop.Run();
  }
#endif

 private:
  static void MaybeQuit(base::OnceClosure& quit) {
    if (quit) {
      std::move(quit).Run();
    }
  }

  mojo::Receiver<mojom::ServiceObserver> receiver_{this};

  std::vector<PurchasedStateCall> purchased_state_calls_;
  base::OnceClosure purchased_state_quit_;

#if !BUILDFLAG(IS_ANDROID)
  std::vector<mojom::ConnectionState> connection_state_calls_;
  std::vector<mojom::RegionPtr> selected_region_calls_;
  std::vector<bool> smart_proxy_calls_;
  base::OnceClosure connection_state_quit_;
  base::OnceClosure selected_region_quit_;
  base::OnceClosure smart_proxy_quit_;
#endif
};

}  // namespace

class BraveVpnServiceTest : public testing::Test {
 public:
  BraveVpnServiceTest() = default;
  ~BraveVpnServiceTest() override = default;

  void SetUp() override { service_ = std::make_unique<TestBraveVpnService>(); }

  void TearDown() override { service_.reset(); }

  TestBraveVpnService* service() { return service_.get(); }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  std::unique_ptr<TestBraveVpnService> service_;
};

// AddObserver should register the observer such that subsequent
// NotifyPurchasedStateChanged calls reach it with the exact state and
// description payload.
TEST_F(BraveVpnServiceTest, AddObserverReceivesPurchasedStateNotification) {
  TestServiceObserver observer;
  service()->AddObserver(observer.BindAndGetRemote());

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         "purchased-from-test");
  observer.WaitForNextPurchasedStateCall();

  ASSERT_EQ(observer.purchased_state_calls().size(), 1u);
  EXPECT_EQ(observer.purchased_state_calls()[0].state,
            mojom::PurchasedState::PURCHASED);
  ASSERT_TRUE(observer.purchased_state_calls()[0].description.has_value());
  EXPECT_EQ(*observer.purchased_state_calls()[0].description,
            "purchased-from-test");
}

// The optional description argument should round-trip a std::nullopt value
// unchanged (i.e. arrive at the observer as an empty optional, not an empty
// string).
TEST_F(BraveVpnServiceTest, PurchasedStateNotificationPreservesNullopt) {
  TestServiceObserver observer;
  service()->AddObserver(observer.BindAndGetRemote());

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::NOT_PURCHASED,
                                         std::nullopt);
  observer.WaitForNextPurchasedStateCall();

  ASSERT_EQ(observer.purchased_state_calls().size(), 1u);
  EXPECT_EQ(observer.purchased_state_calls()[0].state,
            mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(observer.purchased_state_calls()[0].description.has_value());
}

// All registered observers should fan-out receive notifications.
TEST_F(BraveVpnServiceTest, NotificationFansOutToAllObservers) {
  TestServiceObserver observer1;
  TestServiceObserver observer2;
  TestServiceObserver observer3;

  service()->AddObserver(observer1.BindAndGetRemote());
  service()->AddObserver(observer2.BindAndGetRemote());
  service()->AddObserver(observer3.BindAndGetRemote());

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         std::nullopt);

  observer1.WaitForNextPurchasedStateCall();
  observer2.WaitForNextPurchasedStateCall();
  observer3.WaitForNextPurchasedStateCall();

  EXPECT_EQ(observer1.purchased_state_calls().size(), 1u);
  EXPECT_EQ(observer2.purchased_state_calls().size(), 1u);
  EXPECT_EQ(observer3.purchased_state_calls().size(), 1u);
}

// When an observer's mojo pipe is closed, the service should drop it from the
// RemoteSet and stop delivering notifications to it, while continuing to
// deliver to the remaining live observers.
TEST_F(BraveVpnServiceTest, DisconnectedObserverStopsReceivingNotifications) {
  TestServiceObserver observer1;
  TestServiceObserver observer2;
  service()->AddObserver(observer1.BindAndGetRemote());
  service()->AddObserver(observer2.BindAndGetRemote());

  // Close observer1's end of the pipe. Regardless of whether the RemoteSet
  // has processed the disconnect by the time we notify, mojo drops messages
  // sent on a Remote whose receiver is gone, so observer1 cannot be reached.
  observer1.ResetReceiver();

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         std::nullopt);
  observer2.WaitForNextPurchasedStateCall();

  EXPECT_TRUE(observer1.purchased_state_calls().empty());
  EXPECT_EQ(observer2.purchased_state_calls().size(), 1u);
}

// Shutdown must clear all registered observers so that subsequent notifies
// become no-ops. (This holds on every platform.)
TEST_F(BraveVpnServiceTest, ShutdownClearsObservers) {
  TestServiceObserver observer;
  service()->AddObserver(observer.BindAndGetRemote());

  // Shutdown destroys every Remote in the RemoteSet, closing each pipe. The
  // receiver on the other end then observes a disconnect — use that callback
  // as our explicit "Shutdown has fully propagated" signal.
  base::RunLoop disconnect_loop;
  observer.set_disconnect_handler(disconnect_loop.QuitClosure());

  service()->Shutdown();
  disconnect_loop.Run();

  // observers_ was cleared synchronously by Shutdown, so this notify finds an
  // empty set and is a no-op — no message is ever placed on any pipe.
  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         std::nullopt);

  EXPECT_TRUE(observer.purchased_state_calls().empty());
}

// BindInterface should let a caller drive the service over a real mojo pipe;
// AddObserver invoked on that remote must reach the underlying service.
TEST_F(BraveVpnServiceTest, BindInterfaceAcceptsRemoteAddObserverCall) {
  mojo::Remote<mojom::ServiceHandler> handler_remote;
  service()->BindInterface(handler_remote.BindNewPipeAndPassReceiver());

  TestServiceObserver observer;
  handler_remote->AddObserver(observer.BindAndGetRemote());
  handler_remote.FlushForTesting();

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         std::nullopt);
  observer.WaitForNextPurchasedStateCall();

  EXPECT_EQ(observer.purchased_state_calls().size(), 1u);
}

// BindInterface should support multiple independent receivers, and a single
// notify call must reach observers added through any of them.
TEST_F(BraveVpnServiceTest, BindInterfaceSupportsMultipleReceivers) {
  mojo::Remote<mojom::ServiceHandler> remote_a;
  mojo::Remote<mojom::ServiceHandler> remote_b;
  service()->BindInterface(remote_a.BindNewPipeAndPassReceiver());
  service()->BindInterface(remote_b.BindNewPipeAndPassReceiver());

  TestServiceObserver observer_a;
  TestServiceObserver observer_b;
  remote_a->AddObserver(observer_a.BindAndGetRemote());
  remote_b->AddObserver(observer_b.BindAndGetRemote());
  remote_a.FlushForTesting();
  remote_b.FlushForTesting();

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         std::nullopt);
  observer_a.WaitForNextPurchasedStateCall();
  observer_b.WaitForNextPurchasedStateCall();

  EXPECT_EQ(observer_a.purchased_state_calls().size(), 1u);
  EXPECT_EQ(observer_b.purchased_state_calls().size(), 1u);
}

#if !BUILDFLAG(IS_ANDROID)

// Connection-state changes must propagate the exact enum value to observers.
TEST_F(BraveVpnServiceTest, NotifyConnectionStateChangedDeliversState) {
  TestServiceObserver observer;
  service()->AddObserver(observer.BindAndGetRemote());

  service()->NotifyConnectionStateChanged(mojom::ConnectionState::CONNECTED);
  observer.WaitForNextConnectionStateCall();

  service()->NotifyConnectionStateChanged(mojom::ConnectionState::DISCONNECTED);
  observer.WaitForNextConnectionStateCall();

  ASSERT_EQ(observer.connection_state_calls().size(), 2u);
  EXPECT_EQ(observer.connection_state_calls()[0],
            mojom::ConnectionState::CONNECTED);
  EXPECT_EQ(observer.connection_state_calls()[1],
            mojom::ConnectionState::DISCONNECTED);
}

// NotifySelectedRegionChanged should deliver region data to observers and must
// clone (rather than consume) the region for each observer, so multiple
// observers all receive a non-null region. If the implementation were to
// std::move() instead of Clone() inside the loop, the second observer would
// receive a null RegionPtr and this test would fail.
TEST_F(BraveVpnServiceTest, NotifySelectedRegionChangedClonesPerObserver) {
  TestServiceObserver observer1;
  TestServiceObserver observer2;
  service()->AddObserver(observer1.BindAndGetRemote());
  service()->AddObserver(observer2.BindAndGetRemote());

  auto region = mojom::Region::New();
  service()->NotifySelectedRegionChanged(std::move(region));

  observer1.WaitForNextSelectedRegionCall();
  observer2.WaitForNextSelectedRegionCall();

  ASSERT_EQ(observer1.selected_region_calls().size(), 1u);
  ASSERT_EQ(observer2.selected_region_calls().size(), 1u);
  EXPECT_TRUE(observer1.selected_region_calls()[0]);
  EXPECT_TRUE(observer2.selected_region_calls()[0]);
}

// Smart-proxy routing state changes should propagate the boolean unchanged in
// both directions.
TEST_F(BraveVpnServiceTest,
       NotifySmartProxyRoutingStateChangedDeliversBothStates) {
  TestServiceObserver observer;
  service()->AddObserver(observer.BindAndGetRemote());

  service()->NotifySmartProxyRoutingStateChanged(true);
  observer.WaitForNextSmartProxyCall();
  service()->NotifySmartProxyRoutingStateChanged(false);
  observer.WaitForNextSmartProxyCall();

  ASSERT_EQ(observer.smart_proxy_calls().size(), 2u);
  EXPECT_TRUE(observer.smart_proxy_calls()[0]);
  EXPECT_FALSE(observer.smart_proxy_calls()[1]);
}

// On desktop, Shutdown clears bound receivers, which must disconnect the
// remote on the other end of the pipe.
TEST_F(BraveVpnServiceTest, ShutdownDisconnectsReceiversOnDesktop) {
  mojo::Remote<mojom::ServiceHandler> handler_remote;
  service()->BindInterface(handler_remote.BindNewPipeAndPassReceiver());
  handler_remote.FlushForTesting();
  ASSERT_TRUE(handler_remote.is_connected());

  base::RunLoop disconnect_loop;
  handler_remote.set_disconnect_handler(disconnect_loop.QuitClosure());

  service()->Shutdown();
  disconnect_loop.Run();

  EXPECT_FALSE(handler_remote.is_connected());
}

#else  // BUILDFLAG(IS_ANDROID)

// MakeRemote returns a fresh PendingRemote whose receiver end is registered
// with the service, so calls on it must reach the service.
TEST_F(BraveVpnServiceTest, MakeRemoteProducesUsableHandler) {
  mojo::Remote<mojom::ServiceHandler> handler_remote(service()->MakeRemote());
  handler_remote.FlushForTesting();
  ASSERT_TRUE(handler_remote.is_connected());

  TestServiceObserver observer;
  handler_remote->AddObserver(observer.BindAndGetRemote());
  handler_remote.FlushForTesting();

  service()->NotifyPurchasedStateChanged(mojom::PurchasedState::PURCHASED,
                                         std::nullopt);
  observer.WaitForNextPurchasedStateCall();

  EXPECT_EQ(observer.purchased_state_calls().size(), 1u);
}

#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_vpn

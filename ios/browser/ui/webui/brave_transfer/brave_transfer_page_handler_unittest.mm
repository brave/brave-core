// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_transfer/brave_transfer_page_handler.h"

#import <Foundation/Foundation.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.objc+private.h"
#include "brave/components/brave_transfer/ios/browser/transfer_page_handler_bridge.h"
#include "brave/ios/browser/ui/webui/brave_transfer/brave_transfer_page_handler_bridge_holder.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "ios/web/public/test/web_task_environment.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

// A bridge whose PageHandler methods are the generated block adapters, plus
// recording of page connections.
@interface TestTransferBridge : BraveTransferTestPageHandler <
                                    TransferPageHandlerBridge>
@property(nonatomic, nullable) id<BraveTransferPage> connectedPage;
@property(nonatomic) NSInteger disconnectCount;
@end

@implementation TestTransferBridge
- (void)pageDidConnect:(id<BraveTransferPage>)page {
  self.connectedPage = page;
}
- (void)pageDidDisconnect:(id<BraveTransferPage>)page {
  if (page == self.connectedPage) {
    self.connectedPage = nil;
  }
  self.disconnectCount++;
}
@end

namespace brave_transfer {

namespace {

class FakePage : public mojom::Page {
 public:
  explicit FakePage(mojo::PendingReceiver<mojom::Page> receiver)
      : receiver_(this, std::move(receiver)) {}

  void OnServiceStateChanged(mojom::ServiceStatePtr state) override {}
  void OnDevicesChanged(std::vector<mojom::DevicePtr> devices) override {
    last_devices = std::move(devices);
  }
  void OnOfferReceived(mojom::IncomingOfferPtr offer) override {}
  void OnOfferRemoved(const std::string& offer_id) override {
    removed_offer_id = offer_id;
  }
  void OnTransferUpdated(mojom::TransferPtr transfer) override {}
  void OnInboxChanged() override { ++inbox_changed_count; }
  void OnSettingsChanged(mojom::SettingsPtr settings) override {}
  void OnPermissionsChanged(mojom::PermissionsPtr permissions) override {}

  std::optional<std::vector<mojom::DevicePtr>> last_devices;
  std::string removed_offer_id;
  int inbox_changed_count = 0;

 private:
  mojo::Receiver<mojom::Page> receiver_;
};

}  // namespace

class BraveTransferPageHandlerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    PageHandlerBridgeHolder::CreateForWebState(&web_state_);
  }

  void AttachBridge() {
    bridge_ = [[TestTransferBridge alloc] init];
    PageHandlerBridgeHolder::FromWebState(&web_state_)->SetBridge(bridge_);
  }

  void CreateHandler() {
    mojo::PendingRemote<mojom::Page> page_remote;
    page_ = std::make_unique<FakePage>(
        page_remote.InitWithNewPipeAndPassReceiver());
    handler_ = std::make_unique<BraveTransferPageHandler>(
        &web_state_, handler_remote_.BindNewPipeAndPassReceiver(),
        std::move(page_remote));
  }

  web::WebTaskEnvironment task_environment_;
  web::FakeWebState web_state_;
  TestTransferBridge* bridge_ = nil;
  std::unique_ptr<FakePage> page_;
  mojo::Remote<mojom::PageHandler> handler_remote_;
  std::unique_ptr<BraveTransferPageHandler> handler_;
};

TEST_F(BraveTransferPageHandlerTest, RepliesWithDefaultsWithoutBridge) {
  CreateHandler();

  base::test::TestFuture<std::vector<mojom::DevicePtr>> devices;
  handler_remote_->GetDevices(devices.GetCallback());
  EXPECT_TRUE(devices.Get().empty());

  base::test::TestFuture<bool> removed;
  handler_remote_->RemoveDevice("guid", removed.GetCallback());
  EXPECT_FALSE(removed.Get());

  base::test::TestFuture<mojom::SendResult> sent;
  handler_remote_->SendText("guid", "hello", sent.GetCallback());
  EXPECT_EQ(sent.Get(), mojom::SendResult::kFailed);

  base::test::TestFuture<std::vector<mojom::InboxItemPtr>, bool> inbox;
  handler_remote_->GetInbox(0, 50, inbox.GetCallback());
  EXPECT_TRUE(std::get<0>(inbox.Get()).empty());
  EXPECT_FALSE(std::get<1>(inbox.Get()));
}

TEST_F(BraveTransferPageHandlerTest, ForwardsCallsAndConvertsReplies) {
  AttachBridge();
  __block NSString* sent_device = nil;
  __block NSString* sent_text = nil;
  bridge_._sendText = ^(NSString* device_id, NSString* text,
                        void (^completion)(BraveTransferSendResult)) {
    sent_device = device_id;
    sent_text = text;
    completion(BraveTransferSendResultDeviceUnavailable);
  };
  bridge_._devices = ^(void (^completion)(NSArray<BraveTransferDevice*>*)) {
    BraveTransferDevice* device = [[BraveTransferDevice alloc] init];
    device.id = @"guid-1";
    device.name = @"MacBook";
    device.type = BraveTransferDeviceTypeLaptop;
    device.isConnected = true;
    completion(@[ device ]);
  };
  CreateHandler();

  base::test::TestFuture<mojom::SendResult> sent;
  handler_remote_->SendText("guid-1", "https://brave.com", sent.GetCallback());
  EXPECT_EQ(sent.Get(), mojom::SendResult::kDeviceUnavailable);
  EXPECT_NSEQ(sent_device, @"guid-1");
  EXPECT_NSEQ(sent_text, @"https://brave.com");

  base::test::TestFuture<std::vector<mojom::DevicePtr>> devices;
  handler_remote_->GetDevices(devices.GetCallback());
  ASSERT_EQ(devices.Get().size(), 1u);
  EXPECT_EQ(devices.Get()[0]->id, "guid-1");
  EXPECT_EQ(devices.Get()[0]->name, "MacBook");
  EXPECT_EQ(devices.Get()[0]->type, mojom::DeviceType::kLaptop);
  EXPECT_TRUE(devices.Get()[0]->is_connected);
}

TEST_F(BraveTransferPageHandlerTest, PassesSettingsToBridge) {
  AttachBridge();
  __block BraveTransferSettings* received = nil;
  bridge_._updateSettings =
      ^(BraveTransferSettings* settings, void (^completion)(bool)) {
        received = settings;
        completion(true);
      };
  CreateHandler();

  auto settings = mojom::Settings::New();
  settings->receive_mode = mojom::ReceiveMode::kOff;
  settings->auto_accept_trusted = true;
  base::test::TestFuture<bool> updated;
  handler_remote_->UpdateSettings(std::move(settings), updated.GetCallback());
  EXPECT_TRUE(updated.Get());
  ASSERT_TRUE(received);
  EXPECT_EQ(received.receiveMode, BraveTransferReceiveModeOff);
  EXPECT_TRUE(received.autoAcceptTrusted);
}

TEST_F(BraveTransferPageHandlerTest, DroppedCompletionRunsDefaultReply) {
  AttachBridge();
  bridge_._respondToOffer =
      ^(NSString* offer_id, bool accept, void (^completion)(bool)) {
        // Never calls `completion`.
      };
  CreateHandler();

  base::test::TestFuture<bool> responded;
  handler_remote_->RespondToOffer("offer", true, responded.GetCallback());
  EXPECT_FALSE(responded.Get());
}

TEST_F(BraveTransferPageHandlerTest, CompletionCalledTwiceIsIgnored) {
  AttachBridge();
  bridge_._clearHistory = ^(void (^completion)(bool)) {
    completion(true);
    completion(false);
  };
  CreateHandler();

  base::test::TestFuture<bool> cleared;
  handler_remote_->ClearHistory(cleared.GetCallback());
  EXPECT_TRUE(cleared.Get());
}

TEST_F(BraveTransferPageHandlerTest, DesktopOnlyMethodsAreNotSupported) {
  AttachBridge();
  CreateHandler();

  base::test::TestFuture<mojom::SendResult> sent;
  handler_remote_->SendFiles("guid", {}, sent.GetCallback());
  EXPECT_EQ(sent.Get(), mojom::SendResult::kNotSupported);

  base::test::TestFuture<bool> changed;
  handler_remote_->ChooseSaveDirectory(changed.GetCallback());
  EXPECT_FALSE(changed.Get());
}

TEST_F(BraveTransferPageHandlerTest, BridgeEventsReachThePage) {
  AttachBridge();
  CreateHandler();
  ASSERT_TRUE(bridge_.connectedPage);

  BraveTransferDevice* device = [[BraveTransferDevice alloc] init];
  device.id = @"guid-2";
  device.name = @"Desktop";
  [bridge_.connectedPage onDevicesChanged:@[ device ]];
  [bridge_.connectedPage onOfferRemoved:@"offer-1"];
  [bridge_.connectedPage onInboxChanged];

  EXPECT_TRUE(base::test::RunUntil([&] { return page_->inbox_changed_count; }));
  ASSERT_TRUE(page_->last_devices);
  ASSERT_EQ(page_->last_devices->size(), 1u);
  EXPECT_EQ((*page_->last_devices)[0]->id, "guid-2");
  EXPECT_EQ(page_->removed_offer_id, "offer-1");
}

TEST_F(BraveTransferPageHandlerTest, DisconnectsPageWhenDestroyed) {
  AttachBridge();
  CreateHandler();
  ASSERT_TRUE(bridge_.connectedPage);

  handler_.reset();
  EXPECT_FALSE(bridge_.connectedPage);
  EXPECT_EQ(bridge_.disconnectCount, 1);
}

}  // namespace brave_transfer

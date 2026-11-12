/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_fetcher.h"

#include <memory>
#include <utility>

#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/render_frame_host_test_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

class MockOrderExtractor : public mojom::OrderExtractor {
 public:
  MOCK_METHOD(void,
              ExtractOrderInfo,
              (mojom::OrderPatternsInfoPtr, ExtractOrderInfoCallback),
              (override));

  // Mirrors `OrderExtractor::BindReceiver` resetting its receiver before
  // rebinding, which closes the pipe of a fetch already in flight rather
  // than hitting the "already bound" `DCHECK`.
  void Bind(mojo::ScopedMessagePipeHandle handle) {
    receiver_.reset();
    receiver_.Bind(
        mojo::PendingReceiver<mojom::OrderExtractor>(std::move(handle)));
  }

  void ResetReceiver() { receiver_.reset(); }

 private:
  mojo::Receiver<mojom::OrderExtractor> receiver_{this};
};

}  // namespace

class BraveAdsOrderFetcherTest : public ChromeRenderViewHostTestHarness {
 public:
  BraveAdsOrderFetcherTest()
      : ChromeRenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    fetcher_ = std::make_unique<OrderFetcher>(*web_contents());
  }

  MockOrderExtractor* BindMockOrderExtractor() {
    content::RenderFrameHostTester::For(web_contents()->GetPrimaryMainFrame())
        ->InitializeRenderFrameIfNeeded();

    auto mock_order_extractor = std::make_unique<MockOrderExtractor>();
    MockOrderExtractor* const order_extractor_ptr = mock_order_extractor.get();
    mock_order_extractor_ = std::move(mock_order_extractor);

    service_manager::InterfaceProvider::TestApi test_api(
        web_contents()->GetPrimaryMainFrame()->GetRemoteInterfaces());
    test_api.SetBinderForName(
        mojom::OrderExtractor::Name_,
        base::BindRepeating(&MockOrderExtractor::Bind,
                            base::Unretained(order_extractor_ptr)));

    return order_extractor_ptr;
  }

  std::unique_ptr<OrderFetcher> fetcher_;
  std::unique_ptr<MockOrderExtractor> mock_order_extractor_;
};

TEST_F(BraveAdsOrderFetcherTest, DoNotFetchOrderIfRenderFrameNotLive) {
  // Arrange
  NavigateAndCommit(GURL("https://zalando.de/checkout"));

  // Act
  base::test::TestFuture<mojom::OrderInfoPtr> future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       future.GetCallback());

  // Assert
  EXPECT_FALSE(future.Take());
}

TEST_F(BraveAdsOrderFetcherTest, FetchOrder) {
  // Arrange
  NavigateAndCommit(GURL("https://zalando.de/checkout"));

  MockOrderExtractor* const order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(*order_extractor, ExtractOrderInfo)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_patterns*/,
                   mojom::OrderExtractor::ExtractOrderInfoCallback callback) {
        mojom::OrderInfoPtr mojom_order = mojom::OrderInfo::New();
        mojom_order->id = "10105744386682";
        mojom_order->value = "29,85";
        std::move(callback).Run(std::move(mojom_order));
      });

  // Act
  base::test::TestFuture<mojom::OrderInfoPtr> future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       future.GetCallback());

  // Assert
  const mojom::OrderInfoPtr mojom_order = future.Take();
  ASSERT_TRUE(mojom_order);
  EXPECT_EQ("10105744386682", mojom_order->id);
  EXPECT_EQ("29,85", mojom_order->value);
}

// Rebinding the extractor resets the renderer's receiver, disconnecting a
// fetch already in flight. The caller must still be answered.
TEST_F(BraveAdsOrderFetcherTest, CallbackRunWhenExtractorDisconnects) {
  // Arrange
  NavigateAndCommit(GURL("https://zalando.de/checkout"));

  MockOrderExtractor* const order_extractor = BindMockOrderExtractor();

  // Hold the reply, then close the pipe without answering, as the renderer's
  // receiver reset does.
  mojom::OrderExtractor::ExtractOrderInfoCallback pending_reply;
  EXPECT_CALL(*order_extractor, ExtractOrderInfo)
      .WillOnce([&pending_reply](
                    mojom::OrderPatternsInfoPtr /*mojom_patterns*/,
                    mojom::OrderExtractor::ExtractOrderInfoCallback callback) {
        pending_reply = std::move(callback);
      });

  // Act
  base::test::TestFuture<mojom::OrderInfoPtr> future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil([&] { return !pending_reply.is_null(); }));

  order_extractor->ResetReceiver();
  pending_reply.Reset();

  // Assert
  EXPECT_FALSE(future.Take());
}

// A slow-but-still-connected renderer (pipe never closes, reply never
// comes) must still be answered once `kFetchOrderTimeout` elapses, rather
// than leaving the caller waiting indefinitely.
TEST_F(BraveAdsOrderFetcherTest, CallbackRunWhenExtractorReplyTimesOut) {
  // Arrange
  NavigateAndCommit(GURL("https://zalando.de/checkout"));

  MockOrderExtractor* const order_extractor = BindMockOrderExtractor();

  // Hold the reply indefinitely, simulating a slow renderer that never
  // disconnects.
  mojom::OrderExtractor::ExtractOrderInfoCallback pending_reply;
  EXPECT_CALL(*order_extractor, ExtractOrderInfo)
      .WillOnce([&pending_reply](
                    mojom::OrderPatternsInfoPtr /*mojom_patterns*/,
                    mojom::OrderExtractor::ExtractOrderInfoCallback callback) {
        pending_reply = std::move(callback);
      });

  // Act
  base::test::TestFuture<mojom::OrderInfoPtr> future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil([&] { return !pending_reply.is_null(); }));

  task_environment()->FastForwardBy(OrderFetcher::kFetchOrderTimeout);

  // Assert
  EXPECT_FALSE(future.Take());

  // `pending_reply` is still a live response callback for an open pipe;
  // dropping it unanswered, as it would be at scope exit otherwise,
  // crashes. Close the pipe first, mirroring a real disconnect.
  order_extractor->ResetReceiver();
  pending_reply.Reset();
}

// The real reply beating the timeout must stop the timer, so a late-firing
// timeout never runs `callback` a second time.
TEST_F(BraveAdsOrderFetcherTest, DoNotTimeOutAfterExtractorAlreadyReplied) {
  // Arrange
  NavigateAndCommit(GURL("https://zalando.de/checkout"));

  MockOrderExtractor* const order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(*order_extractor, ExtractOrderInfo)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_patterns*/,
                   mojom::OrderExtractor::ExtractOrderInfoCallback callback) {
        std::move(callback).Run(mojom::OrderInfo::New());
      });

  // Act
  base::test::TestFuture<mojom::OrderInfoPtr> future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       future.GetCallback());
  ASSERT_TRUE(future.Wait());

  // Assert, the timer firing after the real reply must not crash or
  // double-invoke `future`'s callback.
  task_environment()->FastForwardBy(OrderFetcher::kFetchOrderTimeout);
}

// A second `FetchOrder` call for a page this `OrderFetcher` no longer cares
// about must resolve the first call's callback immediately with a null
// `OrderInfo`, rather than leaving it to race the second fetch.
TEST_F(BraveAdsOrderFetcherTest, CancelPreviousFetchOnNewFetch) {
  // Arrange
  NavigateAndCommit(GURL("https://zalando.de/checkout"));

  MockOrderExtractor* const order_extractor = BindMockOrderExtractor();

  // Hold the first call's reply indefinitely, simulating a fetch still in
  // flight when the second call arrives.
  mojom::OrderExtractor::ExtractOrderInfoCallback first_pending_reply;
  EXPECT_CALL(*order_extractor, ExtractOrderInfo)
      .WillOnce([&first_pending_reply](
                    mojom::OrderPatternsInfoPtr /*mojom_patterns*/,
                    mojom::OrderExtractor::ExtractOrderInfoCallback callback) {
        first_pending_reply = std::move(callback);
      })
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_patterns*/,
                   mojom::OrderExtractor::ExtractOrderInfoCallback callback) {
        mojom::OrderInfoPtr mojom_order = mojom::OrderInfo::New();
        mojom_order->id = "10105744386682";
        std::move(callback).Run(std::move(mojom_order));
      });

  // Act
  base::test::TestFuture<mojom::OrderInfoPtr> first_future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       first_future.GetCallback());
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !first_pending_reply.is_null(); }));

  base::test::TestFuture<mojom::OrderInfoPtr> second_future;
  fetcher_->FetchOrder(mojom::OrderExtractionPatternsInfo::New(),
                       second_future.GetCallback());

  // Assert, the superseded first fetch resolves immediately with null,
  // while the second fetch still gets its own real reply.
  EXPECT_FALSE(first_future.Take());
  const mojom::OrderInfoPtr second_mojom_order = second_future.Take();
  ASSERT_TRUE(second_mojom_order);
  EXPECT_EQ("10105744386682", second_mojom_order->id);

  // `first_pending_reply` is still a live response callback for an open
  // pipe; dropping it unanswered, as it would be at scope exit otherwise,
  // crashes.
  order_extractor->ResetReceiver();
  first_pending_reply.Reset();
}

}  // namespace brave_ads

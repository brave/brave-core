/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_fetch_helper.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/content/browser/test/mock_order_extractor.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kTimeout = base::Milliseconds(500);
}  // namespace

class BraveAdsOrderFetchHelperTest : public ChromeRenderViewHostTestHarness {
 public:
  BraveAdsOrderFetchHelperTest()
      : ChromeRenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  test::MockOrderExtractor& BindMockOrderExtractor() {
    content::RenderFrameHostTester::For(web_contents()->GetPrimaryMainFrame())
        ->InitializeRenderFrameIfNeeded();

    auto mock_order_extractor = std::make_unique<test::MockOrderExtractor>();
    test::MockOrderExtractor* const order_extractor_ptr =
        mock_order_extractor.get();
    mock_order_extractor_ = std::move(mock_order_extractor);

    service_manager::InterfaceProvider::TestApi test_api(
        web_contents()->GetPrimaryMainFrame()->GetRemoteInterfaces());
    test_api.SetBinderForName(
        mojom::OrderExtractor::Name_,
        base::BindRepeating(&test::MockOrderExtractor::Bind,
                            base::Unretained(order_extractor_ptr)));

    return *order_extractor_ptr;
  }

  std::unique_ptr<test::MockOrderExtractor> mock_order_extractor_;
};

TEST_F(BraveAdsOrderFetchHelperTest, DoesNotFetchOrderIfRenderFrameNotLive) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  // Act
  base::test::TestFuture<mojom::ExtractedOrderInfoPtr> test_future;
  const std::unique_ptr<OrderFetchHelper> order_fetch_helper =
      std::make_unique<OrderFetchHelper>();
  order_fetch_helper->FetchOrder(*web_contents()->GetPrimaryMainFrame(),
                                 mojom::OrderPatternsInfo::New(), kTimeout,
                                 test_future.GetCallback());

  // Assert
  const mojom::ExtractedOrderInfoPtr& mojom_extracted_order = test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/std::nullopt, /*value=*/std::nullopt,
                                   /*currency=*/std::nullopt));
}

TEST_F(BraveAdsOrderFetchHelperTest, FetchOrder) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->id = "10105744386682";
        mojom_extracted_order->value = "29,85";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<mojom::ExtractedOrderInfoPtr> test_future;
  const std::unique_ptr<OrderFetchHelper> order_fetch_helper =
      std::make_unique<OrderFetchHelper>();
  order_fetch_helper->FetchOrder(*web_contents()->GetPrimaryMainFrame(),
                                 mojom::OrderPatternsInfo::New(), kTimeout,
                                 test_future.GetCallback());

  // Assert
  const mojom::ExtractedOrderInfoPtr& mojom_extracted_order = test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/"10105744386682", /*value=*/"29,85",
                                   /*currency=*/std::nullopt));
}

TEST_F(BraveAdsOrderFetchHelperTest, CallbackRunWhenExtractorDisconnects) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();

  // Store the pending reply so it can be reset later to simulate a disconnect.
  mojom::OrderExtractor::ExtractOrderCallback pending_reply;
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([&pending_reply](
                    mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                    mojom::OrderExtractor::ExtractOrderCallback callback) {
        pending_reply = std::move(callback);
      });

  // Act
  base::test::TestFuture<mojom::ExtractedOrderInfoPtr> test_future;
  const std::unique_ptr<OrderFetchHelper> order_fetch_helper =
      std::make_unique<OrderFetchHelper>();
  order_fetch_helper->FetchOrder(*web_contents()->GetPrimaryMainFrame(),
                                 mojom::OrderPatternsInfo::New(), kTimeout,
                                 test_future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil([&] { return !pending_reply.is_null(); }));

  order_extractor.ResetReceiver();
  pending_reply.Reset();

  // Assert
  const mojom::ExtractedOrderInfoPtr& mojom_extracted_order = test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/std::nullopt, /*value=*/std::nullopt,
                                   /*currency=*/std::nullopt));
}

TEST_F(BraveAdsOrderFetchHelperTest, CallbackRunWhenExtractorReplyTimesOut) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();

  // Hold the pending reply so the extractor never answers and the timeout
  // fires.
  mojom::OrderExtractor::ExtractOrderCallback pending_reply;
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([&pending_reply](
                    mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                    mojom::OrderExtractor::ExtractOrderCallback callback) {
        pending_reply = std::move(callback);
      });

  // Act
  base::test::TestFuture<mojom::ExtractedOrderInfoPtr> test_future;
  const std::unique_ptr<OrderFetchHelper> order_fetch_helper =
      std::make_unique<OrderFetchHelper>();
  order_fetch_helper->FetchOrder(*web_contents()->GetPrimaryMainFrame(),
                                 mojom::OrderPatternsInfo::New(), kTimeout,
                                 test_future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil([&] { return !pending_reply.is_null(); }));

  task_environment()->FastForwardBy(kTimeout);

  // Assert
  const mojom::ExtractedOrderInfoPtr& mojom_extracted_order = test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/std::nullopt, /*value=*/std::nullopt,
                                   /*currency=*/std::nullopt));

  order_extractor.ResetReceiver();
  pending_reply.Reset();
}

TEST_F(BraveAdsOrderFetchHelperTest,
       DoesNotTimeOutAfterExtractorAlreadyReplied) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        std::move(callback).Run(mojom::ExtractedOrderInfo::New());
      });

  // Act
  base::test::TestFuture<mojom::ExtractedOrderInfoPtr> test_future;
  const std::unique_ptr<OrderFetchHelper> order_fetch_helper =
      std::make_unique<OrderFetchHelper>();
  order_fetch_helper->FetchOrder(*web_contents()->GetPrimaryMainFrame(),
                                 mojom::OrderPatternsInfo::New(), kTimeout,
                                 test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());

  // Assert
  task_environment()->FastForwardBy(kTimeout);
}

}  // namespace brave_ads

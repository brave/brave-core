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
#include "brave/components/brave_ads/content/browser/test/mock_order_extractor.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/render_frame_host_test_support.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

class BraveAdsOrderFetcherTest : public ChromeRenderViewHostTestHarness {
 public:
  BraveAdsOrderFetcherTest()
      : ChromeRenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    order_fetcher_ = std::make_unique<OrderFetcher>(*web_contents());
  }

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

  std::unique_ptr<OrderFetcher> order_fetcher_;
  std::unique_ptr<test::MockOrderExtractor> mock_order_extractor_;
};

TEST_F(BraveAdsOrderFetcherTest, ForwardsShouldExtractFlagsToCallback) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        std::move(callback).Run(mojom::ExtractedOrderInfo::New());
      });

  mojom::OrderExtractionPatternsInfoPtr mojom_order_extraction_patterns =
      mojom::OrderExtractionPatternsInfo::New();
  mojom_order_extraction_patterns->should_extract_id = true;
  mojom_order_extraction_patterns->should_extract_value = false;

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom_order_extraction_patterns,
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  EXPECT_TRUE(should_extract_id);
  EXPECT_FALSE(should_extract_value);
}

TEST_F(BraveAdsOrderFetcherTest, RejectsIdNotMatchingValidationPattern) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->id = "not-shaped-like-an-id";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  mojom::OrderExtractionPatternsInfoPtr mojom_order_extraction_patterns =
      mojom::OrderExtractionPatternsInfo::New();
  mojom_order_extraction_patterns->id =
      mojom::OrderIdExtractionPatternsInfo::New();
  mojom_order_extraction_patterns->id->validation_pattern =
      R"RE2(^[A-Za-z0-9]{8}$)RE2";

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom_order_extraction_patterns,
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderFetcherTest, AcceptsIdMatchingValidationPattern) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->id = "ABC12345";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  mojom::OrderExtractionPatternsInfoPtr mojom_order_extraction_patterns =
      mojom::OrderExtractionPatternsInfo::New();
  mojom_order_extraction_patterns->id =
      mojom::OrderIdExtractionPatternsInfo::New();
  mojom_order_extraction_patterns->id->validation_pattern =
      R"RE2(^[A-Za-z0-9]{8}$)RE2";

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom_order_extraction_patterns,
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABC12345", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderFetcherTest, RejectsIdLongerThanMaxFieldLength) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->id = std::string(65, 'A');
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderFetcherTest, RejectsIdThatIsACreditCardNumber) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->id = "4242424242424242";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderFetcherTest, RejectsValueThatIsACreditCardNumber) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->value = "4242 4242 4242 4242";
        mojom_extracted_order->currency = "USD";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_FALSE(mojom_extracted_order->value);
  EXPECT_FALSE(mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderFetcherTest, RejectsValueThatIsNotABareNumber) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->value = "<script>alert(1)</script>";
        mojom_extracted_order->currency = "USD";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_FALSE(mojom_extracted_order->value);
  EXPECT_FALSE(mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderFetcherTest, AcceptsValueThatIsABareNumber) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->value = "29.85";
        mojom_extracted_order->currency = "USD";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.85", *mojom_extracted_order->value);
  ASSERT_TRUE(mojom_extracted_order->currency);
  EXPECT_EQ("USD", *mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderFetcherTest, AcceptsValueWithSpaceGroupingSeparator) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->value = "1 234,56";
        mojom_extracted_order->currency = "EUR";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr> test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             test_future.GetCallback());

  // Assert
  const auto& [should_extract_id, should_extract_value, mojom_extracted_order] =
      test_future.Get();
  ASSERT_TRUE(mojom_extracted_order);
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("1 234,56", *mojom_extracted_order->value);
  ASSERT_TRUE(mojom_extracted_order->currency);
  EXPECT_EQ("EUR", *mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderFetcherTest, AbandonsPreviousFetchOnNewFetch) {
  // Arrange
  NavigateAndCommit(GURL("https://corge.example/checkout"));

  test::MockOrderExtractor& order_extractor = BindMockOrderExtractor();

  // Hold the first call's reply, simulating a fetch still in flight when the
  // second call arrives.
  mojom::OrderExtractor::ExtractOrderCallback first_pending_reply;
  EXPECT_CALL(order_extractor, ExtractOrder)
      .WillOnce([&first_pending_reply](
                    mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                    mojom::OrderExtractor::ExtractOrderCallback callback) {
        first_pending_reply = std::move(callback);
      })
      .WillOnce([](mojom::OrderPatternsInfoPtr /*mojom_order_patterns*/,
                   mojom::OrderExtractor::ExtractOrderCallback callback) {
        mojom::ExtractedOrderInfoPtr mojom_extracted_order =
            mojom::ExtractedOrderInfo::New();
        mojom_extracted_order->id = "10105744386682";
        std::move(callback).Run(std::move(mojom_extracted_order));
      });

  // Act
  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr>
      first_test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             first_test_future.GetCallback());
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !first_pending_reply.is_null(); }));

  base::test::TestFuture<bool, bool, mojom::ExtractedOrderInfoPtr>
      second_test_future;
  order_fetcher_->FetchOrder(*mojom::OrderExtractionPatternsInfo::New(),
                             second_test_future.GetCallback());

  // The abandoned first fetch's extractor still answers, late.
  std::move(first_pending_reply).Run(mojom::ExtractedOrderInfo::New());

  // Assert
  EXPECT_FALSE(first_test_future.IsReady());

  const auto& [second_should_extract_id, second_should_extract_value,
               second_mojom_extracted_order] = second_test_future.Get();
  ASSERT_TRUE(second_mojom_extracted_order);
  EXPECT_THAT(
      *second_mojom_extracted_order,
      ::testing::FieldsAre(/*id=*/"10105744386682", /*value=*/std::nullopt,
                           /*currency=*/std::nullopt));
}

}  // namespace brave_ads

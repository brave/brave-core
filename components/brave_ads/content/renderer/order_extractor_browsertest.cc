/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_extractor.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/test_future.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "content/public/test/render_view_test.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads {

namespace {

mojom::OrderPatternsInfoPtr BuildOrderPatterns() {
  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->number_of_ancestor_levels_to_search = 3;
  return mojom_patterns;
}

mojom::OrderSelectorInfoPtr BuildSelector(const std::string& selector,
                                          std::string attribute_name) {
  mojom::OrderSelectorInfoPtr mojom_selector = mojom::OrderSelectorInfo::New();
  mojom_selector->selector = selector;
  mojom_selector->attribute_name = std::move(attribute_name);
  return mojom_selector;
}

mojom::OrderSelectorInfoPtr BuildSelector(const std::string& selector) {
  mojom::OrderSelectorInfoPtr mojom_selector = mojom::OrderSelectorInfo::New();
  mojom_selector->selector = selector;
  return mojom_selector;
}

}  // namespace

class BraveAdsOrderExtractorRenderViewTest : public content::RenderViewTest {
 public:
  void SetUp() override {
    content::RenderViewTest::SetUp();
    registry_ = std::make_unique<service_manager::BinderRegistry>();
    extractor_ =
        std::make_unique<OrderExtractor>(GetMainRenderFrame(), registry_.get());
  }

  void TearDown() override {
    extractor_.reset();
    registry_.reset();
    content::RenderViewTest::TearDown();
  }

 protected:
  mojom::OrderInfoPtr ExtractOrderInfo(
      mojom::OrderPatternsInfoPtr mojom_patterns) {
    base::test::TestFuture<mojom::OrderInfoPtr> test_future;
    extractor_->ExtractOrderInfo(std::move(mojom_patterns),
                                 test_future.GetCallback());
    return test_future.Take();
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> registry_;
  std::unique_ptr<OrderExtractor> extractor_;
};

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractValueViaAncestorSearch) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Total</span>
      <span>$29.85</span>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_patterns->value_currency_regexes = {{"USD", R"RE2(\$\d+\.\d{2})RE2"}};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  ASSERT_TRUE(mojom_order->value);
  EXPECT_EQ("29.85", *mojom_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoNotExtractValueIfAncestorHasMultipleCandidates) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Total</span>
      <span>$29.85</span>
      <span>$9.99 financing available</span>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_patterns->value_currency_regexes = {{"USD", R"RE2(\$\d+\.\d{2})RE2"}};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_FALSE(mojom_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractValueViaSelector) {
  // Arrange
  LoadHTML(R"HTML(<span class="checkout-total">4,98 €</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->value_currency_regexes = {{"EUR", R"RE2(\d+,\d{2}\s*€)RE2"}};
  mojom_patterns->value_selectors.push_back(BuildSelector(".checkout-total"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  ASSERT_TRUE(mojom_order->value);
  EXPECT_EQ("4,98", *mojom_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       FallBackToHeuristicIfSelectorElementMissing) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Total</span>
      <span>$29.85</span>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_patterns->value_currency_regexes = {{"USD", R"RE2(\$\d+\.\d{2})RE2"}};
  mojom_patterns->value_selectors.push_back(BuildSelector(".checkout-total"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  ASSERT_TRUE(mojom_order->value);
  EXPECT_EQ("29.85", *mojom_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractIdViaRegex) {
  // Arrange
  LoadHTML(R"HTML(<span>Order Number: ABC123456</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->id_regexes = {R"RE2(Order Number: (\w+))RE2"};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  ASSERT_TRUE(mojom_order->id);
  EXPECT_EQ("ABC123456", *mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractIdViaElementTextFallback) {
  // Arrange, label and ID split across sibling elements, so no single text
  // node contains both and the regex can only match combined element text.
  LoadHTML(
      R"HTML(<span>Confirmation number:</span><span>ABC123456</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  ASSERT_TRUE(mojom_order->id);
  EXPECT_EQ("ABC123456", *mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractIdViaSelectorAttribute) {
  // Arrange
  LoadHTML(R"HTML(<div data-order-id="XYZ789">thank you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->id_selectors.push_back(
      BuildSelector("[data-order-id]", "data-order-id"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  ASSERT_TRUE(mojom_order->id);
  EXPECT_EQ("XYZ789", *mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, DoNotExtractAnythingWhenNoMatch) {
  // Arrange
  LoadHTML(R"HTML(<p>Nothing relevant on this page.</p>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = BuildOrderPatterns();
  mojom_patterns->id_regexes = {R"RE2(Order Number: (\w+))RE2"};
  mojom_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_patterns->value_currency_regexes = {{"USD", R"RE2(\$\d+\.\d{2})RE2"}};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_FALSE(mojom_order->id);
  EXPECT_FALSE(mojom_order->value);
}

}  // namespace brave_ads

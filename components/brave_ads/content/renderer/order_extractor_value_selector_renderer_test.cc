/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor.h"
#include "brave/components/brave_ads/content/renderer/test/order_value_extraction_patterns_test_constants.h"
#include "content/public/test/render_view_test.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

mojom::OrderPatternsInfoPtr BuildOrderPatterns() {
  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->number_of_ancestor_levels_to_search = 3;
  mojom_order_patterns->id_max_combined_text_length_to_search = 1024;
  mojom_order_patterns->id_number_of_descendant_levels_to_search = 32;
  return mojom_order_patterns;
}

mojom::OrderSelectorInfoPtr BuildSelector(
    const std::string& selector,
    std::optional<std::string> attribute_name) {
  mojom::OrderSelectorInfoPtr mojom_order_selector =
      mojom::OrderSelectorInfo::New();
  mojom_order_selector->selector = selector;
  mojom_order_selector->attribute_name = std::move(attribute_name);
  return mojom_order_selector;
}

}  // namespace

struct ValueSelectorTestCase {
  std::string test_name;
  std::string text;
  std::optional<std::string> expected_value;
  std::optional<std::string> expected_currency;
};

class BraveAdsOrderExtractorValueSelectorTest
    : public content::RenderViewTest,
      public ::testing::WithParamInterface<ValueSelectorTestCase> {
 public:
  void SetUp() override {
    content::RenderViewTest::SetUp();
    registry_ = std::make_unique<service_manager::BinderRegistry>();
    order_extractor_ =
        std::make_unique<OrderExtractor>(GetMainRenderFrame(), registry_.get());
  }

  void TearDown() override {
    order_extractor_.reset();
    registry_.reset();
    content::RenderViewTest::TearDown();
  }

 protected:
  mojom::ExtractedOrderInfoPtr ExtractOrder(
      mojom::OrderPatternsInfoPtr mojom_order_patterns) {
    base::test::TestFuture<mojom::ExtractedOrderInfoPtr> test_future;
    order_extractor_->ExtractOrder(std::move(mojom_order_patterns),
                                   test_future.GetCallback());
    return test_future.Take();
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> registry_;
  std::unique_ptr<OrderExtractor> order_extractor_;
};

TEST_P(BraveAdsOrderExtractorValueSelectorTest, ExtractsExpectedValue) {
  const ValueSelectorTestCase& test_case = GetParam();

  // Arrange
  LoadHTML(base::StrCat({R"HTML(<span class="checkout-total">)HTML",
                         test_case.text, R"HTML(</span>)HTML"}));

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_EQ(test_case.expected_value, mojom_extracted_order->value);
  EXPECT_EQ(test_case.expected_currency, mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderExtractorValueSelectorTest,
       ExtractsCurrencylessNumberWithNoOtherText) {
  // Arrange
  LoadHTML(R"HTML(<span class="checkout-total">29.99</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.99", *mojom_extracted_order->value);
  EXPECT_FALSE(mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderExtractorValueSelectorTest,
       ExtractsCurrencylessNumberWithCommaDecimalAndNoOtherText) {
  // Arrange
  LoadHTML(R"HTML(<span class="checkout-total">29,99</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29,99", *mojom_extracted_order->value);
  EXPECT_FALSE(mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderExtractorValueSelectorTest,
       DoesNotExtractCurrencylessNumberWithTrailingPunctuation) {
  // Arrange
  LoadHTML(R"HTML(<span class="checkout-total">29.99.</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorValueSelectorTest,
       DoesNotGuessCurrencyForBareNumberWhenCurrencyRegexesAreConfigured) {
  // Arrange
  LoadHTML(R"HTML(<span class="checkout-total">29.99</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.99", *mojom_extracted_order->value);
  EXPECT_FALSE(mojom_extracted_order->currency);
}

TEST_F(BraveAdsOrderExtractorValueSelectorTest,
       PrefersLaterCurrencyTaggedMatchOverEarlierCurrencylessMatch) {
  // Arrange
  LoadHTML(R"HTML(<span class="bare-total">29.99</span>
                  <span class="tagged-total">19.99 USD</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".bare-total", std::nullopt));
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".tagged-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("19.99", *mojom_extracted_order->value);
  ASSERT_TRUE(mojom_extracted_order->currency);
  EXPECT_EQ("USD", *mojom_extracted_order->currency);
}

std::string ValueSelectorTestCaseName(
    const ::testing::TestParamInfo<ValueSelectorTestCase>& info) {
  return info.param.test_name;
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsOrderExtractorValueSelectorTest,
                         ::testing::ValuesIn(std::vector<ValueSelectorTestCase>{
                             {.test_name = "LeadingEuroSymbol",
                              .text = "€29,99",
                              .expected_value = "29,99",
                              .expected_currency = "EUR"},
                             {.test_name = "TrailingEuroSymbol",
                              .text = "29,99 €",
                              .expected_value = "29,99",
                              .expected_currency = "EUR"},
                             {.test_name = "LeadingDollarSymbol",
                              .text = "$29.99",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "LeadingUsDollarSymbol",
                              .text = "US$29.99",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "TrailingUsDollarSymbol",
                              .text = "29.99 US$",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "LeadingAndTrailingUsd",
                              .text = "$29.99 USD",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "TrailingUppercaseUsd",
                              .text = "29.99 USD",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "TrailingLowercaseUsd",
                              .text = "29.99 usd",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "LeadingPoundSymbol",
                              .text = "£19.99",
                              .expected_value = "19.99",
                              .expected_currency = "GBP"},
                             {.test_name = "LeadingUppercaseGbp",
                              .text = "GBP 19.99",
                              .expected_value = "19.99",
                              .expected_currency = "GBP"},
                             {.test_name = "LeadingLowercaseGbp",
                              .text = "gbp 19.99",
                              .expected_value = "19.99",
                              .expected_currency = "GBP"},
                             {.test_name = "TrailingUppercaseGbp",
                              .text = "19.99 GBP",
                              .expected_value = "19.99",
                              .expected_currency = "GBP"},
                             {.test_name = "TrailingLowercaseGbp",
                              .text = "29.99 gbp",
                              .expected_value = "29.99",
                              .expected_currency = "GBP"},
                             {.test_name = "GroupingWithGbp",
                              .text = "£1,234.56",
                              .expected_value = "1,234.56",
                              .expected_currency = "GBP"},
                             {.test_name = "LeadingLowercaseUsd",
                              .text = "usd 29.99",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "LeadingUppercaseEur",
                              .text = "EUR 29,99",
                              .expected_value = "29,99",
                              .expected_currency = "EUR"},
                             {.test_name = "LeadingLowercaseEur",
                              .text = "eur 29,99",
                              .expected_value = "29,99",
                              .expected_currency = "EUR"},
                             {.test_name = "TrailingLowercaseEur",
                              .text = "29,99 eur",
                              .expected_value = "29,99",
                              .expected_currency = "EUR"},
                             {.test_name = "LabelBeforeDollar",
                              .text = "Total price: $29.99",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "LabelBeforePound",
                              .text = "Price: £19.99",
                              .expected_value = "19.99",
                              .expected_currency = "GBP"},
                             {.test_name = "TrailingTaxNote",
                              .text = "€29,99 incl. VAT",
                              .expected_value = "29,99",
                              .expected_currency = "EUR"},
                             {.test_name = "LabelBeforeTrailingEur",
                              .text = "Amount to pay: 29.99 EUR",
                              .expected_value = "29.99",
                              .expected_currency = "EUR"},
                             {.test_name = "GroupingCommaDecimalPoint",
                              .text = "1,234.56 USD",
                              .expected_value = "1,234.56",
                              .expected_currency = "USD"},
                             {.test_name = "GroupingWithNoDecimalPart",
                              .text = "$1,234",
                              .expected_value = "1,234",
                              .expected_currency = "USD"},
                             {.test_name = "SingleDigitAmount",
                              .text = "$5",
                              .expected_value = "5",
                              .expected_currency = "USD"},
                             {.test_name = "GroupingPointDecimalComma",
                              .text = "1.234,56 EUR",
                              .expected_value = "1.234,56",
                              .expected_currency = "EUR"},
                             {.test_name = "GroupingRegularSpace",
                              .text = "1 234,56 €",
                              .expected_value = "1 234,56",
                              .expected_currency = "EUR"},
                             {.test_name = "GroupingNonBreakingSpace",
                              .text = "1 234,56 €",
                              .expected_value = "1 234,56",
                              .expected_currency = "EUR"},
                             {.test_name = "LeadingAndTrailingWhitespace",
                              .text = "   $29.99   ",
                              .expected_value = "29.99",
                              .expected_currency = "USD"},
                             {.test_name = "ItemNumber",
                              .text = "Item 12345",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                             {.test_name = "ReferenceCode",
                              .text = "Reference ABC12345",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                             {.test_name = "OrderId",
                              .text = "Order ID: 12345678",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                             {.test_name = "ProductCode",
                              .text = "Product code 987654",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                             {.test_name = "Loading",
                              .text = "Loading...",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                             {.test_name = "Unavailable",
                              .text = "Unavailable",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                             {.test_name = "NoPriceAvailable",
                              .text = "No price available",
                              .expected_value = std::nullopt,
                              .expected_currency = std::nullopt},
                         }),
                         &ValueSelectorTestCaseName);

}  // namespace brave_ads

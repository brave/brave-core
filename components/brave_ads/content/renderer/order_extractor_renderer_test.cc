/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/test/test_future.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor.h"
#include "brave/components/brave_ads/content/renderer/test/html_test_util.h"
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

class BraveAdsOrderExtractorRenderViewTest : public content::RenderViewTest {
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

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractValueViaAncestorSearch) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Total</span>
      <span>$29.85</span>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", R"RE2(\$\d+\.\d{2})RE2"}};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.85", *mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotExtractValueIfAncestorHasMultipleCandidates) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Total</span>
      <span>$29.85</span>
      <span>$9.99 financing available</span>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", R"RE2(\$\d+\.\d{2})RE2"}};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractValueViaSelector) {
  // Arrange
  LoadHTML(R"HTML(<span class="checkout-total">4,98 €</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes = {
      {"EUR", R"RE2(\d+,\d{2}\s*€)RE2"}};
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("4,98", *mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       FallBackToHeuristicIfSelectorElementMissing) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Total</span>
      <span>$29.85</span>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", R"RE2(\$\d+\.\d{2})RE2"}};
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.85", *mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractIdViaRegex) {
  // Arrange
  LoadHTML(R"HTML(<span>Order Number: ABC123456</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Order Number: (\w+))RE2"};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABC123456", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractIdViaElementTextFallback) {
  // Arrange
  LoadHTML(
      R"HTML(<span>Confirmation number:</span><span>CNF55102AB</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("CNF55102AB", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaElementTextFallbackWithNestedDomWithinDepthBound) {
  // Arrange
  LoadHTML(test::WrapInNestedDivs(
      /*nesting_depth=*/20,
      R"HTML(<span>Confirmation number:</span><span>QZ80214XY</span>)HTML"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("QZ80214XY", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotCrashOnExtremelyDeeplyNestedDom) {
  // Arrange
  LoadHTML(test::WrapInNestedDivs(
      /*nesting_depth=*/50000,
      R"HTML(<span>Confirmation number:</span><span>ABC123456</span>)HTML"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};

  // Act & Assert
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotExtractIdViaElementTextFallbackBeyondDepthBound) {
  // Arrange
  LoadHTML(test::WrapInNestedDivs(
      /*nesting_depth=*/33,
      R"HTML(<span>Confirmation number:</span><span>ABC123456</span>)HTML"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaElementTextFallbackIgnoresMatchedDescendantsAncestor) {
  // Arrange
  LoadHTML(R"HTML(
    <div>
      <span>Confirmation number:</span>
      <div>
        <span>Confirmation number:</span><span>ORD9981234</span>
      </div>
    </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ORD9981234", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotExtractIdViaElementTextFallbackBeyondTextLengthLimit) {
  // Arrange
  LoadHTML(
      R"HTML(<div>
        <span>Confirmation number:</span><span>ABC123456</span>
      </div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Confirmation number:\s*(\w+))RE2"};
  mojom_order_patterns->id_max_combined_text_length_to_search = 10;

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaElementTextFallbackExactlyAtTextLengthLimit) {
  // Arrange
  LoadHTML(R"HTML(<span>ID:</span><span>ABCDEFGH</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(ID:(\w+))RE2"};
  mojom_order_patterns->id_max_combined_text_length_to_search = 11;

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABCDEFGH", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotAcceptIdTruncatedByTextLengthLimit) {
  // Arrange
  LoadHTML(R"HTML(<span>ID:</span><span>ABCDEFGH</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(ID:(\w+))RE2"};
  mojom_order_patterns->id_max_combined_text_length_to_search = 10;

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotAcceptIdTruncatedMidMultibyteUtf8CharacterByTextLengthLimit) {
  // Arrange
  LoadHTML(R"HTML(<span>ID:</span><span>€X</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(ID:(.+))RE2"};
  mojom_order_patterns->id_max_combined_text_length_to_search = 5;

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest, ExtractIdViaSelectorAttribute) {
  // Arrange
  LoadHTML(R"HTML(<div data-order-id="XYZ789">thank you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_selectors.push_back(
      BuildSelector("[data-order-id]", "data-order-id"));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("XYZ789", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaSelectorWithOwnValidationPatternAccepted) {
  // Arrange
  LoadHTML(R"HTML(<div data-order-id="ABCDEF0123456789ABCDEF01">thank
           you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom::OrderSelectorInfoPtr mojom_order_selector =
      BuildSelector("[data-order-id]", "data-order-id");
  mojom_order_selector->validation_pattern = "^[A-Fa-f0-9]{24}$";
  mojom_order_patterns->id_selectors.push_back(std::move(mojom_order_selector));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABCDEF0123456789ABCDEF01", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotExtractIdViaSelectorWhenOwnValidationPatternRejectsMatch) {
  // Arrange
  LoadHTML(R"HTML(<div data-order-id="not-a-valid-shape">thank you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom::OrderSelectorInfoPtr mojom_order_selector =
      BuildSelector("[data-order-id]", "data-order-id");
  mojom_order_selector->validation_pattern = "^[A-Fa-f0-9]{24}$";
  mojom_order_patterns->id_selectors.push_back(std::move(mojom_order_selector));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaSelectorFallsBackToDomainValidationPattern) {
  // Arrange
  LoadHTML(R"HTML(<div data-order-id="ABCD1234">thank you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_validation_pattern = "^[A-Za-z0-9]{8}$";
  mojom_order_patterns->id_selectors.push_back(
      BuildSelector("[data-order-id]", "data-order-id"));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABCD1234", *mojom_extracted_order->id);
}

TEST_F(
    BraveAdsOrderExtractorRenderViewTest,
    DoesNotExtractIdViaSelectorWhenDomainValidationPatternRejectsFallbackMatch) {
  // Arrange
  LoadHTML(R"HTML(<div data-order-id="too-short">thank you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_validation_pattern = "^[A-Za-z0-9]{8}$";
  mojom_order_patterns->id_selectors.push_back(
      BuildSelector("[data-order-id]", "data-order-id"));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaGenericRegexWithMultipleCaptureGroups) {
  // Arrange
  LoadHTML(R"HTML(<span>Order Number: ABC-123456</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Order Number: ([A-Z]+)-(\d+))RE2"};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABC123456", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdViaSelectorPatternWithMultipleCaptureGroups) {
  // Arrange
  LoadHTML(R"HTML(<script>var order = "xvpn:sub:ABC-123456";</script>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom::OrderSelectorInfoPtr mojom_order_selector =
      BuildSelector("script:not([src])", /*attribute_name=*/std::nullopt);
  mojom_order_selector->pattern = R"RE2(xvpn:sub:([A-Z]+)-(\d+))RE2";
  mojom_order_patterns->id_selectors.push_back(std::move(mojom_order_selector));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABC123456", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractFirstValidIdWhenAnEarlierCandidateFailsValidation) {
  // Arrange
  LoadHTML(R"HTML(
    <span>Order Number: TOOSHORT</span>
    <span>Order Number: ABC123456</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Order Number: (\w+))RE2"};
  mojom_order_patterns->id_validation_pattern = "^[A-Za-z0-9]{9}$";

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABC123456", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractIdFromSecondSelectorAfterFirstFailsValidation) {
  // Arrange
  LoadHTML(R"HTML(
    <div data-order-id="not-a-valid-shape">thank you</div>
    <div data-backup-id="ABC12345">thank you</div>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom::OrderSelectorInfoPtr first_mojom_order_selector =
      BuildSelector("[data-order-id]", "data-order-id");
  first_mojom_order_selector->validation_pattern = "^[A-Fa-f0-9]{24}$";
  mojom_order_patterns->id_selectors.push_back(
      std::move(first_mojom_order_selector));
  mojom::OrderSelectorInfoPtr second_mojom_order_selector =
      BuildSelector("[data-backup-id]", "data-backup-id");
  second_mojom_order_selector->validation_pattern = "^[A-Za-z0-9]{8}$";
  mojom_order_patterns->id_selectors.push_back(
      std::move(second_mojom_order_selector));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->id);
  EXPECT_EQ("ABC12345", *mojom_extracted_order->id);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractValueFromSecondSelectorAfterFirstIsNonNumeric) {
  // Arrange
  LoadHTML(R"HTML(
    <span class="shipping-note">Ships in 2 days</span>
    <span class="checkout-total">4,98 €</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes = {
      {"EUR", R"RE2(\d+,\d{2}\s*€)RE2"}};
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".shipping-note", std::nullopt));
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("4,98", *mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractsLastOfMultipleNumbersWhenCurrencyPresent) {
  // Arrange
  LoadHTML(
      R"HTML(<span class="checkout-total">Was €39.99 now €29.99</span>)HTML");

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
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotExtractEitherOfMultipleNumbersWithoutCurrency) {
  // Arrange
  LoadHTML(
      R"HTML(<span class="checkout-total">Was 39.99 now 29.99</span>)HTML");

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

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractValueFromSecondMatchingElementAfterFirstIsInvalid) {
  // Arrange
  LoadHTML(R"HTML(
    <span class="checkout-total">Item 12345</span>
    <span class="checkout-total">$29.99</span>)HTML");

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
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractValueFromSecondSelectorAfterFirstIsAnUnrelatedItemNumber) {
  // Arrange
  LoadHTML(R"HTML(
    <span class="item-number">Item 12345</span>
    <span class="checkout-total">$29.99</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".item-number", std::nullopt));
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.99", *mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       ExtractValueFromSecondSelectorAfterFirstIsAnUnrelatedReferenceCode) {
  // Arrange
  LoadHTML(R"HTML(
    <span class="reference">Reference ABC12345</span>
    <span class="checkout-total">$29.99</span>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".reference", std::nullopt));
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(".checkout-total", std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  ASSERT_TRUE(mojom_extracted_order->value);
  EXPECT_EQ("29.99", *mojom_extracted_order->value);
}

TEST_F(BraveAdsOrderExtractorRenderViewTest,
       DoesNotExtractAnythingWhenNoMatch) {
  // Arrange
  LoadHTML(R"HTML(<p>Nothing relevant on this page.</p>)HTML");

  mojom::OrderPatternsInfoPtr mojom_order_patterns = BuildOrderPatterns();
  mojom_order_patterns->id_regexes = {R"RE2(Order Number: (\w+))RE2"};
  mojom_order_patterns->value_keyword_regexes = {"(?i)^Total$"};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", R"RE2(\$\d+\.\d{2})RE2"}};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_FALSE(mojom_extracted_order->id);
  EXPECT_FALSE(mojom_extracted_order->value);
}

}  // namespace brave_ads

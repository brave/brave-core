/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor.h"
#include "brave/components/brave_ads/content/renderer/test/order_id_extraction_patterns_test_constants.h"
#include "brave/components/brave_ads/content/renderer/test/order_value_extraction_patterns_test_constants.h"
#include "content/public/test/render_view_test.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

base::FilePath GetTestDataDir() {
  return base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
      .AppendASCII("brave")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("brave_ads")
      .AppendASCII("conversions")
      .AppendASCII("order");
}

std::string ReadHtmlFile(const std::string& relative_path) {
  base::FilePath path = GetTestDataDir();
  for (const std::string& component :
       base::SplitString(relative_path, "/", base::TRIM_WHITESPACE,
                         base::SPLIT_WANT_NONEMPTY)) {
    path = path.AppendASCII(component);
  }
  std::string contents;
  CHECK(base::ReadFileToString(path, &contents));
  return contents;
}

mojom::OrderSelectorInfoPtr BuildSelector(
    const std::string& selector,
    std::optional<std::string> attribute_name,
    std::optional<std::string> pattern) {
  mojom::OrderSelectorInfoPtr mojom_order_selector =
      mojom::OrderSelectorInfo::New();
  mojom_order_selector->selector = selector;
  mojom_order_selector->attribute_name = std::move(attribute_name);
  mojom_order_selector->pattern = std::move(pattern);
  return mojom_order_selector;
}

}  // namespace

class BraveAdsOrderExtractorSiteFixturesRenderViewTest
    : public content::RenderViewTest {
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

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueTaggedWithDataAttributeAndNoId) {
  // Arrange
  LoadHTML(ReadHtmlFile("value_tagged_with_data_attribute_and_no_id.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_selectors.push_back(BuildSelector(
      R"CSS([data-test="checkout-total"])CSS", /*attribute_name=*/std::nullopt,
      /*pattern=*/std::nullopt));
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/std::nullopt,
                  /*value=*/::testing::Optional(std::string("4,98")),
                  /*currency=*/::testing::Optional(std::string("EUR"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueTaggedWithDataAttributeAndIdInHeading) {
  // Arrange
  LoadHTML(ReadHtmlFile(
      "de/value_tagged_with_data_attribute_and_id_in_heading.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_selectors.push_back(BuildSelector(
      R"CSS([data-test="checkout-total"])CSS", /*attribute_name=*/std::nullopt,
      /*pattern=*/std::nullopt));
  mojom_order_patterns->id_regexes = {test::kDeOrderIdRegex};
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/::testing::Optional(std::string("424242424")),
                  /*value=*/::testing::Optional(std::string("4,98")),
                  /*currency=*/::testing::Optional(std::string("EUR"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueTaggedWithDataAttributeAndIdReadFromAttribute) {
  // Arrange
  LoadHTML(ReadHtmlFile(
      "value_tagged_with_data_attribute_and_id_read_from_attribute.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector("[data-grand-total]", /*attribute_name=*/std::nullopt,
                    /*pattern=*/std::nullopt));
  mojom_order_patterns->id_selectors.push_back(
      BuildSelector("[data-shopping-cart-id]", "data-shopping-cart-id",
                    /*pattern=*/std::nullopt));
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(
      *mojom_extracted_order,
      ::testing::FieldsAre(
          /*id=*/::testing::Optional(std::string("42c0ffee42c0ffee42c0ffee")),
          /*value=*/::testing::Optional(std::string("13,37")),
          /*currency=*/::testing::Optional(std::string("EUR"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       NoValueFoundButIdReadFromAttribute) {
  // Arrange
  LoadHTML(ReadHtmlFile("no_value_found_but_id_read_from_attribute.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->id_selectors.push_back(BuildSelector(
      "[data-order-id]", "data-order-id", /*pattern=*/std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(
      *mojom_extracted_order,
      ::testing::FieldsAre(
          /*id=*/::testing::Optional(std::string("42c0ffee42c0ffee42c0ffee")),
          /*value=*/std::nullopt,
          /*currency=*/std::nullopt));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueWithHiddenDuplicateCopy) {
  // Arrange
  LoadHTML(ReadHtmlFile("value_with_hidden_duplicate_copy.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_selectors.push_back(BuildSelector(
      R"CSS(button[aria-label="Preisdetails"] > span > span:nth-of-type(2):not([aria-hidden]))CSS",
      /*attribute_name=*/std::nullopt, /*pattern=*/std::nullopt));
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/std::nullopt,
                  /*value=*/::testing::Optional(std::string("417,25")),
                  /*currency=*/::testing::Optional(std::string("EUR"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueInDollarsWithGermanNumberFormatAndIdInPanel) {
  // Arrange
  LoadHTML(ReadHtmlFile(
      "de/value_in_dollars_with_german_number_format_and_id_in_panel.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {
      test::kDeOrderValueKeywordRegex};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", test::kUsdOrderValueCurrencyRegex}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;
  mojom_order_patterns->id_selectors.push_back(BuildSelector(
      R"CSS([data-testid="booking-actions-reference-number"] [role="button"] div)CSS",
      /*attribute_name=*/std::nullopt, /*pattern=*/std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/::testing::Optional(std::string("9305817264")),
                  /*value=*/::testing::Optional(std::string("470,12")),
                  /*currency=*/::testing::Optional(std::string("USD"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueInDollarsWithGermanNumberFormatAndUnrelatedPopupOnPage) {
  // Arrange
  LoadHTML(
      ReadHtmlFile("de/"
                   "value_in_dollars_with_german_number_format_and_unrelated_"
                   "popup_on_page.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {
      test::kDeOrderValueKeywordRegex};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", test::kUsdOrderValueCurrencyRegex}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;
  mojom_order_patterns->id_selectors.push_back(BuildSelector(
      R"CSS([data-testid="booking-actions-reference-number"] [role="button"] div)CSS",
      /*attribute_name=*/std::nullopt, /*pattern=*/std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/::testing::Optional(std::string("9305817264")),
                  /*value=*/::testing::Optional(std::string("470,12")),
                  /*currency=*/::testing::Optional(std::string("USD"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest, ValueInEurosAndNoId) {
  // Arrange
  LoadHTML(ReadHtmlFile("de/value_in_euros_and_no_id.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {
      test::kDeOrderValueKeywordRegex};
  mojom_order_patterns->value_currency_regexes = {
      {"EUR", test::kEurOrderValueCurrencyRegex}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/std::nullopt, /*value=*/"24,99",
                                   /*currency=*/"EUR"));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       NoValueFoundButIdViaSelector) {
  // Arrange
  LoadHTML(ReadHtmlFile("no_value_found_but_id_via_selector.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {
      test::kEnOrderValueKeywordRegex};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", test::kUsdOrderValueCurrencyRegex}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;
  mojom_order_patterns->id_selectors.push_back(BuildSelector(
      R"CSS([data-testid="orderNumber"])CSS", /*attribute_name=*/std::nullopt,
      /*pattern=*/std::nullopt));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/"42424242424242",
                                   /*value=*/std::nullopt,
                                   /*currency=*/std::nullopt));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueInSummaryBoxWithIdOnSameLine) {
  // Arrange
  LoadHTML(ReadHtmlFile("de/value_in_summary_box_with_id_on_same_line.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_selectors.push_back(
      BuildSelector(R"CSS(aside[data-inspector-id="orderSummary"] strong)CSS",
                    /*attribute_name=*/std::nullopt, /*pattern=*/std::nullopt));
  mojom_order_patterns->id_regexes = {test::kDeOrderIdRegex};
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/"FOOBARBAZ", /*value=*/"19,95",
                                   /*currency=*/"EUR"));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueOnOneLineWithIdSplitAcrossElements) {
  // Arrange
  LoadHTML(
      ReadHtmlFile("en/value_on_one_line_with_id_split_across_elements.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {R"RE2((?i)^Total price:)RE2"};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", R"RE2((?:US\$|USD|\$)\s*\d+\.\d{2})RE2"}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;
  mojom_order_patterns->id_regexes = {test::kEnOrderIdRegex};
  mojom_order_patterns->id_max_combined_text_length_to_search = 4096;
  mojom_order_patterns->id_number_of_descendant_levels_to_search =
      test::kDefaultOrderIdMaxDescendantLevelsToSearch;

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(/*id=*/"77318495026", /*value=*/"88.17",
                                   /*currency=*/"USD"));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       IdBuriedInScriptAmongManyEmptyScripts) {
  // Arrange
  LoadHTML(ReadHtmlFile("id_buried_in_script_among_many_empty_scripts.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->id_selectors.push_back(BuildSelector(
      "script:not([src])", /*attribute_name=*/std::nullopt,
      /*pattern=*/
      R"RE2((?i)checkout:sub:([0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}))RE2"));

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/"deadbeef-dead-beef-dead-beefdeadbeef",
                  /*value=*/std::nullopt, /*currency=*/std::nullopt));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueBuriedInScriptWithIdInText) {
  // Arrange
  LoadHTML(ReadHtmlFile("en/value_buried_in_script_with_id_in_text.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_selectors.push_back(BuildSelector(
      "script:not([src])", /*attribute_name=*/std::nullopt,
      /*pattern=*/
      R"RE2((?i)"orderTotal"\s*:\s*([0-9]+(?:\.[0-9]{2})?)\s*,\s*"currency"\s*:\s*"([A-Z]{3})")RE2"));
  mojom_order_patterns->id_regexes = {test::kEnOrderIdRegex};
  mojom_order_patterns->value_currency_regexes =
      test::OrderValueCurrencyRegexes();

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/::testing::Optional(std::string("ORD03-507281924617")),
                  /*value=*/::testing::Optional(std::string("258.56")),
                  /*currency=*/::testing::Optional(std::string("USD"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueInSummaryRowWithIdInText) {
  // Arrange
  LoadHTML(ReadHtmlFile("en/value_in_summary_row_with_id_in_text.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {
      test::kEnOrderValueKeywordRegex};
  mojom_order_patterns->value_currency_regexes = {
      {"USD", test::kUsdOrderValueCurrencyRegex}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;
  mojom_order_patterns->id_regexes = {test::kEnOrderIdRegex};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/::testing::Optional(std::string("ORD03-507281924617")),
                  /*value=*/::testing::Optional(std::string("145.20")),
                  /*currency=*/::testing::Optional(std::string("USD"))));
}

TEST_F(BraveAdsOrderExtractorSiteFixturesRenderViewTest,
       ValueInEurosWithFrenchNumberFormatAndIdInText) {
  // Arrange
  LoadHTML(ReadHtmlFile(
      "fr/value_in_euros_with_french_number_format_and_id_in_text.html"));

  mojom::OrderPatternsInfoPtr mojom_order_patterns =
      mojom::OrderPatternsInfo::New();
  mojom_order_patterns->value_keyword_regexes = {
      test::kFrOrderValueKeywordRegex};
  mojom_order_patterns->value_currency_regexes = {
      {"EUR", test::kEurOrderValueCurrencyRegex}};
  mojom_order_patterns->number_of_ancestor_levels_to_search =
      test::kDefaultOrderValueMaxAncestorLevelsToSearch;
  mojom_order_patterns->id_regexes = {test::kFrOrderIdRegex};

  // Act
  const mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      ExtractOrder(std::move(mojom_order_patterns));
  ASSERT_TRUE(mojom_extracted_order);

  // Assert
  EXPECT_THAT(*mojom_extracted_order,
              ::testing::FieldsAre(
                  /*id=*/::testing::Optional(std::string("ABC1234567")),
                  /*value=*/::testing::Optional(std::string("1 234,56")),
                  /*currency=*/::testing::Optional(std::string("EUR"))));
}

}  // namespace brave_ads

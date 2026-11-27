/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_patterns_mojom_util.h"

#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderPatternsMojomUtilTest, ConvertBothIdAndValuePatterns) {
  // Arrange
  mojom::OrderExtractionPatternsInfoPtr mojom_order_extraction_patterns =
      mojom::OrderExtractionPatternsInfo::New();

  mojom::OrderIdExtractionPatternsInfoPtr mojom_order_id_extraction_patterns =
      mojom::OrderIdExtractionPatternsInfo::New();
  mojom_order_id_extraction_patterns->regexes = {
      "(?i)^Bestellnummer:\\s*(\\w+)$"};
  mojom_order_id_extraction_patterns->max_combined_text_length_to_search = 4096;
  mojom::OrderExtractionSelectorInfoPtr mojom_order_id_extraction_selector =
      mojom::OrderExtractionSelectorInfo::New();
  mojom_order_id_extraction_selector->selector = "[data-order-id]";
  mojom_order_id_extraction_selector->attribute_name = "data-order-id";
  mojom_order_id_extraction_patterns->selectors.push_back(
      std::move(mojom_order_id_extraction_selector));
  mojom_order_extraction_patterns->id =
      std::move(mojom_order_id_extraction_patterns);

  mojom::OrderValueExtractionPatternsInfoPtr
      mojom_order_value_extraction_patterns =
          mojom::OrderValueExtractionPatternsInfo::New();
  mojom_order_value_extraction_patterns->currency_regexes = {
      {"EUR", "(?i)EUR|€"}};
  mojom_order_value_extraction_patterns->keyword_regexes = {"(?i)^Gesamt$"};
  mojom_order_value_extraction_patterns->number_of_ancestor_levels_to_search =
      6;
  mojom::OrderExtractionSelectorInfoPtr mojom_order_value_extraction_selector =
      mojom::OrderExtractionSelectorInfo::New();
  mojom_order_value_extraction_selector->selector =
      R"([data-test="checkout-total"])";
  mojom_order_value_extraction_patterns->selectors.push_back(
      std::move(mojom_order_value_extraction_selector));
  mojom_order_extraction_patterns->value =
      std::move(mojom_order_value_extraction_patterns);

  // Act
  const mojom::OrderPatternsInfoPtr mojom_order_patterns =
      ToMojomOrderPatterns(*mojom_order_extraction_patterns);

  // Assert
  EXPECT_THAT(
      *mojom_order_patterns,
      ::testing::FieldsAre(
          /*id_selectors=*/::testing::ElementsAre(::testing::Pointee(
              ::testing::FieldsAre(/*selector=*/"[data-order-id]",
                                   /*attribute_name=*/"data-order-id",
                                   /*pattern=*/std::nullopt,
                                   /*validation_pattern=*/std::nullopt))),
          /*id_regexes=*/
          std::vector<std::string>{"(?i)^Bestellnummer:\\s*(\\w+)$"},
          /*id_max_combined_text_length_to_search=*/4096U,
          /*id_number_of_descendant_levels_to_search=*/0U,
          /*id_validation_pattern=*/std::nullopt,
          /*value_selectors=*/
          ::testing::ElementsAre(::testing::Pointee(::testing::FieldsAre(
              /*selector=*/R"([data-test="checkout-total"])",
              /*attribute_name=*/std::nullopt,
              /*pattern=*/std::nullopt,
              /*validation_pattern=*/std::nullopt))),
          /*value_currency_regexes=*/
          (base::flat_map<std::string, std::string>{{"EUR", "(?i)EUR|€"}}),
          /*value_keyword_regexes=*/std::vector<std::string>{"(?i)^Gesamt$"},
          /*number_of_ancestor_levels_to_search=*/6U));
}

TEST(BraveAdsOrderPatternsMojomUtilTest,
     ConvertEmptyOrderPatternsIfNeitherIdNorValueSet) {
  // Arrange
  mojom::OrderExtractionPatternsInfoPtr mojom_order_extraction_patterns =
      mojom::OrderExtractionPatternsInfo::New();

  // Act
  const mojom::OrderPatternsInfoPtr mojom_order_patterns =
      ToMojomOrderPatterns(*mojom_order_extraction_patterns);

  // Assert
  EXPECT_THAT(*mojom_order_patterns,
              ::testing::FieldsAre(
                  /*id_selectors=*/::testing::IsEmpty(),
                  /*id_regexes=*/::testing::IsEmpty(),
                  /*id_max_combined_text_length_to_search=*/0U,
                  /*id_number_of_descendant_levels_to_search=*/0U,
                  /*id_validation_pattern=*/std::nullopt,
                  /*value_selectors=*/::testing::IsEmpty(),
                  /*value_currency_regexes=*/::testing::IsEmpty(),
                  /*value_keyword_regexes=*/::testing::IsEmpty(),
                  /*number_of_ancestor_levels_to_search=*/0U));
}

}  // namespace brave_ads

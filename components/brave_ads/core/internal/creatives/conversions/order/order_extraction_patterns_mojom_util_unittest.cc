/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_mojom_util.h"

#include <optional>
#include <utility>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     ConvertToMojomOrderExtractionPatternsWithNeitherIdNorValueRequested) {
  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_order_patterns =
      ToMojomOrderExtractionPatterns(
          OrderIdExtractionPatternsInfo(), /*should_extract_id=*/false,
          OrderValueExtractionPatternsInfo(), /*should_extract_value=*/false);
  ASSERT_TRUE(mojom_order_patterns);

  // Assert
  EXPECT_THAT(*mojom_order_patterns,
              ::testing::FieldsAre(
                  /*id=*/::testing::_, /*should_extract_id=*/false,
                  /*value=*/::testing::_, /*should_extract_value=*/false));
}

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     ConvertIdOnlyToMojomOrderExtractionPatterns) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = "[data-order-id]";
  order_extraction_selector.attribute_name = "data-order-id";

  const std::string id_regex = "(?i)^Bestellnummer:\\s*(\\w+)$";

  OrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns.regexes = {id_regex};
  order_id_extraction_patterns.selectors = {order_extraction_selector};
  order_id_extraction_patterns.max_combined_text_length_to_search = 4096;

  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_order_patterns =
      ToMojomOrderExtractionPatterns(
          std::move(order_id_extraction_patterns), /*should_extract_id=*/true,
          OrderValueExtractionPatternsInfo(), /*should_extract_value=*/false);
  ASSERT_TRUE(mojom_order_patterns);

  // Assert
  EXPECT_THAT(*mojom_order_patterns,
              ::testing::FieldsAre(
                  /*id=*/::testing::Pointee(::testing::FieldsAre(
                      /*selectors=*/::testing::ElementsAre(
                          ::testing::Pointee(::testing::FieldsAre(
                              /*selector=*/"[data-order-id]",
                              /*attribute_name=*/
                              ::testing::Optional(std::string("data-order-id")),
                              /*pattern=*/std::nullopt,
                              /*validation_pattern=*/std::nullopt))),
                      /*regexes=*/std::vector<std::string>{id_regex},
                      /*max_combined_text_length_to_search=*/4096U,
                      /*number_of_descendant_levels_to_search=*/0U,
                      /*validation_pattern=*/std::nullopt)),
                  /*should_extract_id=*/true,
                  /*value=*/::testing::_, /*should_extract_value=*/false));
}

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     ConvertValueOnlyToMojomOrderExtractionPatterns) {
  // Arrange
  const std::string currency_regex = "(?i)EUR|€";
  const std::string keyword_regex = "(?i)^Gesamt$";

  OrderValueExtractionPatternsInfo order_value_extraction_patterns;
  order_value_extraction_patterns.currency_regexes = {{"EUR", currency_regex}};
  order_value_extraction_patterns.keyword_regexes = {keyword_regex};
  order_value_extraction_patterns.number_of_ancestor_levels_to_search = 6;

  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_order_patterns =
      ToMojomOrderExtractionPatterns(OrderIdExtractionPatternsInfo(),
                                     /*should_extract_id=*/false,
                                     std::move(order_value_extraction_patterns),
                                     /*should_extract_value=*/true);
  ASSERT_TRUE(mojom_order_patterns);

  // Assert
  EXPECT_THAT(
      *mojom_order_patterns,
      ::testing::FieldsAre(
          /*id=*/::testing::_, /*should_extract_id=*/false,
          /*value=*/
          ::testing::Pointee(::testing::FieldsAre(
              /*selectors=*/::testing::IsEmpty(),
              /*currency_regexes=*/
              (base::flat_map<std::string, std::string>{
                  {"EUR", currency_regex}}),
              /*keyword_regexes=*/std::vector<std::string>{keyword_regex},
              /*number_of_ancestor_levels_to_search=*/6U)),
          /*should_extract_value=*/true));
}

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     DoesNotSetMojomAttributeNameWhenNotGiven) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = R"([data-test="checkout-total"])";

  OrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns.selectors = {order_extraction_selector};

  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_order_patterns =
      ToMojomOrderExtractionPatterns(
          std::move(order_id_extraction_patterns), /*should_extract_id=*/true,
          OrderValueExtractionPatternsInfo(), /*should_extract_value=*/false);
  ASSERT_TRUE(mojom_order_patterns);

  // Assert
  ASSERT_TRUE(mojom_order_patterns->id);
  EXPECT_THAT(
      mojom_order_patterns->id->selectors,
      ::testing::ElementsAre(::testing::Pointee(::testing::Field(
          &mojom::OrderExtractionSelectorInfo::attribute_name, std::nullopt))));
}

}  // namespace brave_ads

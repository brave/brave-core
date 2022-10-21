/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/search_result_ad/test_web_page_util.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"

namespace brave_ads {

namespace {

schema_org::mojom::ValuesPtr CreateVectorValuesPtr(std::string value) {
  return schema_org::mojom::Values::NewStringValues({std::move(value)});
}

schema_org::mojom::ValuesPtr CreateVectorValuesPtr(int64_t value) {
  return schema_org::mojom::Values::NewLongValues({value});
}

class TestWebPageConstructor final {
 public:
  explicit TestWebPageConstructor(int attribute_index_to_skip)
      : attribute_index_to_skip_(attribute_index_to_skip) {
    web_page_ = CreateWebPage();
  }
  ~TestWebPageConstructor() = default;
  TestWebPageConstructor(const TestWebPageConstructor&) = delete;
  TestWebPageConstructor& operator=(const TestWebPageConstructor&) = delete;

  blink::mojom::WebPagePtr GetTestWebPage() { return std::move(web_page_); }

 private:
  blink::mojom::WebPagePtr CreateWebPage() {
    blink::mojom::WebPagePtr web_page = blink::mojom::WebPage::New();
    schema_org::mojom::EntityPtr entity = schema_org::mojom::Entity::New();
    entity->type = "Product";
    schema_org::mojom::PropertyPtr property =
        schema_org::mojom::Property::New();
    property->name = "creatives";

    std::vector<schema_org::mojom::EntityPtr> entity_values;
    entity_values.push_back(CreateCreativeEntity());
    property->values =
        schema_org::mojom::Values::NewEntityValues(std::move(entity_values));

    entity->properties.push_back(std::move(property));
    web_page->entities.push_back(std::move(entity));

    return web_page;
  }

  template <typename T>
  void AddProperty(std::vector<schema_org::mojom::PropertyPtr>* properties,
                   base::StringPiece name,
                   T value) {
    const int index = current_attribute_index_++;
    if (index == attribute_index_to_skip_) {
      return;
    }
    schema_org::mojom::PropertyPtr property =
        schema_org::mojom::Property::New();
    property->name = static_cast<std::string>(name);
    property->values = CreateVectorValuesPtr(value);

    properties->push_back(std::move(property));
  }

  schema_org::mojom::EntityPtr CreateCreativeEntity() {
    const char* kSearchResultAdStringAttributes[] = {
        "data-placement-id",
        "data-creative-set-id",
        "data-campaign-id",
        "data-advertiser-id",
        "data-headline-text",
        "data-description",
        "data-conversion-type-value",
        "data-conversion-url-pattern-value",
        "data-conversion-advertiser-public-key-value"};

    schema_org::mojom::EntityPtr entity = schema_org::mojom::Entity::New();
    entity->type = "SearchResultAd";

    AddProperty<std::string>(&entity->properties, "data-creative-instance-id",
                             kCreativeInstanceId);
    AddProperty<std::string>(&entity->properties, "data-landing-page",
                             kTargetUrl);
    AddProperty<std::string>(&entity->properties, "data-rewards-value", "0.5");
    AddProperty<int64_t>(&entity->properties,
                         "data-conversion-observation-window-value", 1);

    int index = 0;
    for (const auto** it = std::begin(kSearchResultAdStringAttributes);
         it != std::end(kSearchResultAdStringAttributes); ++it, ++index) {
      AddProperty<std::string>(
          &entity->properties, *it,
          std::string("value") + base::NumberToString(index));
    }

    return entity;
  }

  blink::mojom::WebPagePtr web_page_;
  int current_attribute_index_ = 0;
  int attribute_index_to_skip_ = -1;
};

}  // namespace

blink::mojom::WebPagePtr CreateTestWebPage(int attribute_index_to_skip) {
  TestWebPageConstructor constructor(attribute_index_to_skip);
  return constructor.GetTestWebPage();
}

}  // namespace brave_ads

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/test_web_page_util.h"

#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "components/schema_org/common/metadata.mojom.h"

namespace brave_ads {

namespace {

constexpr const char* kSearchResultAdStringAttributes[] = {
    "data-creative-instance-id",
    "data-creative-set-id",
    "data-campaign-id",
    "data-advertiser-id",
    "data-headline-text",
    "data-description",
    "data-conversion-url-pattern-value",
    "data-conversion-advertiser-public-key-value"};

schema_org::mojom::ValuesPtr CreateVectorValuesPtr(std::string value) {
  return schema_org::mojom::Values::NewStringValues({std::move(value)});
}

schema_org::mojom::ValuesPtr CreateVectorValuesPtr(int64_t value) {
  return schema_org::mojom::Values::NewLongValues({value});
}

schema_org::mojom::ValuesPtr CreateVectorValuesPtr(bool value) {
  return schema_org::mojom::Values::NewBoolValues({value});
}

class TestWebPageEntitiesConstructor final {
 public:
  explicit TestWebPageEntitiesConstructor(
      std::vector<base::StringPiece> attributes_to_skip)
      : attributes_to_skip_(std::move(attributes_to_skip)) {
    web_page_entities_ = CreateWebPageEntities();
  }

  std::vector<::schema_org::mojom::EntityPtr> GetTestWebPageEntities() {
    return std::move(web_page_entities_);
  }

 private:
  std::vector<::schema_org::mojom::EntityPtr> CreateWebPageEntities() {
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
    std::vector<::schema_org::mojom::EntityPtr> entities;
    entities.push_back(std::move(entity));

    return entities;
  }

  template <typename T>
  void AddProperty(std::vector<schema_org::mojom::PropertyPtr>* properties,
                   base::StringPiece name,
                   T value) {
    if (base::Contains(attributes_to_skip_, name)) {
      return;
    }

    schema_org::mojom::PropertyPtr property =
        schema_org::mojom::Property::New();
    property->name = static_cast<std::string>(name);
    property->values = CreateVectorValuesPtr(value);

    properties->push_back(std::move(property));
  }

  schema_org::mojom::EntityPtr CreateCreativeEntity() {
    schema_org::mojom::EntityPtr entity = schema_org::mojom::Entity::New();
    entity->type = "SearchResultAd";

    AddProperty<std::string>(&entity->properties, "data-placement-id",
                             kTestWebPagePlacementId);
    AddProperty<std::string>(&entity->properties, "data-landing-page",
                             "https://brave.com");
    AddProperty<std::string>(&entity->properties, "data-rewards-value", "0.5");
    AddProperty<int64_t>(&entity->properties,
                         "data-conversion-observation-window-value", 1);
    AddProperty<bool>(&entity->properties,
                      "data-conversion-extract-external-id-value", true);

    // Generate values for simple string properties.
    int index = 0;
    for (const auto* const* it = std::begin(kSearchResultAdStringAttributes);
         it != std::end(kSearchResultAdStringAttributes); ++it, ++index) {
      AddProperty<std::string>(
          &entity->properties, *it,
          std::string("value") + base::NumberToString(index));
    }

    return entity;
  }

  std::vector<::schema_org::mojom::EntityPtr> web_page_entities_;
  std::vector<base::StringPiece> attributes_to_skip_;
};

}  // namespace

std::vector<::schema_org::mojom::EntityPtr> CreateTestWebPageEntities(
    std::vector<base::StringPiece> attributes_to_skip) {
  TestWebPageEntitiesConstructor constructor(std::move(attributes_to_skip));
  return constructor.GetTestWebPageEntities();
}

}  // namespace brave_ads

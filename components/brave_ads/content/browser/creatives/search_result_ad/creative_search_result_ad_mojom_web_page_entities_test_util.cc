/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_test_util.h"

#include <cstdint>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_constants.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_test_util.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_test_constants.h"
#include "components/schema_org/common/metadata.mojom.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"

namespace brave_ads::test {

namespace {

class CreativeAdMojomWebPageEntitiesConstructor final {
 public:
  explicit CreativeAdMojomWebPageEntitiesConstructor(
      std::vector<std::string_view> excluded_property_names)
      : excluded_property_names_(std::move(excluded_property_names)) {
    mojom_web_page_entities_ = CreativeSearchResultAdMojomWebPageEntities();
  }

  std::vector<schema_org::mojom::EntityPtr> GetMojomWebPageEntities() {
    return std::move(mojom_web_page_entities_);
  }

 private:
  std::vector<schema_org::mojom::EntityPtr>
  CreativeSearchResultAdMojomWebPageEntities() {
    schema_org::mojom::EntityPtr mojom_entity =
        schema_org::mojom::Entity::New();
    mojom_entity->type = kCreativeSearchResultAdsProductMojomEntityType;

    schema_org::mojom::PropertyPtr mojom_property =
        schema_org::mojom::Property::New();
    mojom_property->name = kCreativeSearchResultAdsMojomPropertyName;
    std::vector<schema_org::mojom::EntityPtr> mojom_entities;
    mojom_entities.push_back(CreateCreativeAdMojomEntity());
    mojom_property->values =
        schema_org::mojom::Values::NewEntityValues(std::move(mojom_entities));
    mojom_entity->properties.push_back(std::move(mojom_property));

    std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities;
    mojom_web_page_entities.push_back(std::move(mojom_entity));
    return mojom_web_page_entities;
  }

  schema_org::mojom::EntityPtr CreateCreativeAdMojomEntity() {
    schema_org::mojom::EntityPtr mojom_entity =
        schema_org::mojom::Entity::New();

    mojom_entity->type = kCreativeSearchResultAdMojomEntityType;

    // Creative ad.
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdPlacementIdPropertyName,
        kCreativeAdPlacementId);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdCreativeInstanceIdPropertyName,
        kCreativeAdCreativeInstanceId);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdCreativeSetIdPropertyName,
        kCreativeAdCreativeSetId);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdCampaignIdPropertyName,
        kCreativeAdCampaignId);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdAdvertiserIdPropertyName,
        kCreativeAdAdvertiserId);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdLandingPagePropertyName,
        kCreativeAdLandingPage);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdHeadlineTextPropertyName,
        kCreativeAdHeadlineText);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdDescriptionPropertyName,
        kCreativeAdDescription);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeAdRewardsValuePropertyName,
        base::NumberToString(kCreativeAdRewardsValue));

    // Creative set conversion.
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties, kCreativeSetConversionUrlPatternPropertyName,
        kCreativeSetConversionUrlPattern);
    MaybeAddCreativeAdMojomProperty<std::string>(
        &mojom_entity->properties,
        kCreativeSetConversionAdvertiserPublicKeyPropertyName,
        kCreativeSetConversionAdvertiserPublicKey);
    MaybeAddCreativeAdMojomProperty<int64_t>(
        &mojom_entity->properties,
        kCreativeSetConversionObservationWindowPropertyName,
        kCreativeSetConversionObservationWindow.InDays());

    return mojom_entity;
  }

  template <typename T>
  void MaybeAddCreativeAdMojomProperty(
      std::vector<schema_org::mojom::PropertyPtr>* mojom_properties,
      const std::string& name,
      T value) {
    if (!base::Contains(excluded_property_names_, name)) {
      AddMojomProperty<T>(mojom_properties, name, std::move(value));
    }
  }

  const std::vector<std::string_view> excluded_property_names_;

  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities_;
};

}  // namespace

std::vector<schema_org::mojom::EntityPtr>
CreativeSearchResultAdMojomWebPageEntities(
    std::vector<std::string_view> excluded_property_names) {
  CreativeAdMojomWebPageEntitiesConstructor constructor(
      std::move(excluded_property_names));
  return constructor.GetMojomWebPageEntities();
}

blink::mojom::WebPagePtr CreativeSearchResultAdMojomWebPage(
    std::vector<std::string_view> excluded_property_names) {
  blink::mojom::WebPagePtr mojom_web_page = blink::mojom::WebPage::New();
  mojom_web_page->entities = CreativeSearchResultAdMojomWebPageEntities(
      std::move(excluded_property_names));
  return mojom_web_page;
}

std::vector<schema_org::mojom::EntityPtr>
CreativeSearchResultAdMojomWebPageEntitiesWithProperty(std::string_view name,
                                                       std::string_view value) {
  CreativeAdMojomWebPageEntitiesConstructor constructor(
      /*excluded_property_names=*/{name});
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      constructor.GetMojomWebPageEntities();
  const auto& mojom_property = mojom_web_page_entities[0]->properties[0];
  auto& mojom_entity = mojom_property->values->get_entity_values()[0];
  test::AddMojomProperty<std::string>(&mojom_entity->properties,
                                      /*name=*/std::string(name),
                                      /*value=*/std::string(value));
  return mojom_web_page_entities;
}

blink::mojom::WebPagePtr CreativeSearchResultAdMojomWebPageWithProperty(
    std::string_view name,
    std::string_view value) {
  blink::mojom::WebPagePtr mojom_web_page = blink::mojom::WebPage::New();
  mojom_web_page->entities =
      CreativeSearchResultAdMojomWebPageEntitiesWithProperty(name, value);
  return mojom_web_page;
}

}  // namespace brave_ads::test

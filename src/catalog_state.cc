/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog_state.h"
#include "string_helper.h"
#include "json_helper.h"
#include "static_values.h"

namespace ads {

CatalogState::CatalogState() :
    catalog_id(""),
    version(0),
    ping(0),
    campaigns({}),
    issuers(IssuersInfo()) {}

CatalogState::CatalogState(const CatalogState& state) :
    catalog_id(state.catalog_id),
    version(state.version),
    ping(state.ping),
    campaigns(state.campaigns),
    issuers(state.issuers) {}

CatalogState::~CatalogState() = default;

Result CatalogState::FromJson(
    const std::string& json,
    const std::string& json_schema,
    std::string* error_description) {
  rapidjson::Document catalog;
  catalog.Parse(json.c_str());

  auto result = helper::JSON::Validate(&catalog, json_schema);
  if (result != SUCCESS) {
    if (error_description != nullptr) {
      *error_description = helper::JSON::GetLastError(&catalog);
    }

    return result;
  }

  std::string new_catalog_id = "";
  uint64_t new_version = 0;
  uint64_t new_ping = kDefaultCatalogPing * kMillisecondsInASecond;
  std::vector<CampaignInfo> new_campaigns = {};
  IssuersInfo new_issuers = IssuersInfo();

  new_catalog_id = catalog["catalogId"].GetString();

  new_version = catalog["version"].GetUint64();
  if (new_version != 1) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'patch invalid', { reason: 'unsupported version', version: version }
    return SUCCESS;
  }

  new_ping = catalog["ping"].GetUint64();

  // Campaigns
  for (const auto& campaign : catalog["campaigns"].GetArray()) {
    CampaignInfo campaign_info;

    campaign_info.campaign_id = campaign["campaignId"].GetString();
    campaign_info.advertiser_id = campaign["advertiserId"].GetString();
    campaign_info.name = campaign["name"].GetString();
    campaign_info.start_at = campaign["startAt"].GetString();
    campaign_info.end_at = campaign["endAt"].GetString();
    campaign_info.daily_cap = campaign["dailyCap"].GetUint();
    campaign_info.budget = campaign["budget"].GetUint();

    // Geo targets
    for (const auto& geo_target : campaign["geoTargets"].GetArray()) {
      GeoTargetInfo geo_target_info;

      geo_target_info.code = geo_target["code"].GetString();
      geo_target_info.name = geo_target["name"].GetString();

      campaign_info.geo_targets.push_back(geo_target_info);
    }

    // Creative sets
    for (const auto& creative_set : campaign["creativeSets"].GetArray()) {
      CreativeSetInfo creative_set_info;

      creative_set_info.creative_set_id =
          creative_set["creativeSetId"].GetString();

      std::string execution = creative_set["execution"].GetString();
      if (execution != "per_click") {
        if (error_description != nullptr) {
          *error_description = "Catalog invalid: creativeSet has unknown "
              "execution: " + execution;
        }

        return FAILED;
      }
      creative_set_info.execution = execution;

      creative_set_info.per_day = creative_set["perDay"].GetUint();

      creative_set_info.total_max = creative_set["totalMax"].GetUint();

      // Segments
      auto segments = creative_set["segments"].GetArray();
      if (segments.Size() == 0) {
        if (error_description != nullptr) {
          *error_description = "Catalog invalid: No segments for creativeSet "
              "with creativeSetId: " + creative_set_info.creative_set_id;
        }

        return FAILED;
      }

      for (const auto& segment : segments) {
        SegmentInfo segment_info;

        segment_info.code = segment["code"].GetString();
        segment_info.name = segment["name"].GetString();

        creative_set_info.segments.push_back(segment_info);
      }

      // Creatives
      for (const auto& creative : creative_set["creatives"].GetArray()) {
        CreativeInfo creative_info;

        creative_info.creative_instance_id =
            creative["creativeInstanceId"].GetString();

        // Type
        auto type = creative["type"].GetObject();

        creative_info.type.code = type["code"].GetString();

        std::string name = type["name"].GetString();
        if (name != "notification") {
          if (error_description != nullptr) {
            *error_description = "Catalog invalid: Invalid creative type: "
                + name + " for creativeInstanceId: " +
                creative_info.creative_instance_id;
          }

          return FAILED;
        }
        creative_info.type.name = name;

        creative_info.type.platform = type["platform"].GetString();

        creative_info.type.version = type["version"].GetUint64();

        // Payload
        auto payload = creative["payload"].GetObject();

        creative_info.payload.body = payload["body"].GetString();
        creative_info.payload.title = payload["title"].GetString();
        creative_info.payload.target_url = payload["targetUrl"].GetString();

        creative_set_info.creatives.push_back(creative_info);
      }

      campaign_info.creative_sets.push_back(creative_set_info);
    }

    new_campaigns.push_back(campaign_info);
  }

  // Issuers
  for (const auto& issuer : catalog["issuers"].GetArray()) {
    IssuerInfo issuer_info;

    std::string name = issuer["name"].GetString();
    std::string public_key = issuer["publicKey"].GetString();

    if (name == "confirmation") {
      new_issuers.public_key = public_key;
      continue;
    }

    issuer_info.name = name;
    issuer_info.public_key = public_key;

    new_issuers.issuers.push_back(issuer_info);
  }

  catalog_id = new_catalog_id;
  version = new_version;
  ping = new_ping;
  campaigns = new_campaigns;
  issuers = new_issuers;

  return SUCCESS;
}

}  // namespace ads

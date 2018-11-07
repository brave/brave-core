/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog_state.h"
#include "string_helper.h"

namespace ads {

CATALOG_STATE::CATALOG_STATE() :
    catalog_id(""),
    version(0),
    ping(0),
    campaigns({}) {}

CATALOG_STATE::CATALOG_STATE(const CATALOG_STATE& state) {
    catalog_id = state.catalog_id;
    version = state.version;
    ping = state.ping;
    campaigns = state.campaigns;
}

CATALOG_STATE::~CATALOG_STATE() = default;

bool CATALOG_STATE::LoadFromJson(
    const std::string& json,
    const std::string& jsonSchema) {
  rapidjson::Document catalog_schema;
  catalog_schema.Parse(jsonSchema.c_str());

  if (catalog_schema.HasParseError()) {
    return false;
  }

  rapidjson::Document catalog;
  catalog.Parse(json.c_str());

  if (catalog.HasParseError()) {
    return false;
  }

  rapidjson::SchemaDocument schema(catalog_schema);
  rapidjson::SchemaValidator validator(schema);
  if (!catalog.Accept(validator)) {
    return false;
  }

  std::string new_catalog_id;
  uint64_t new_version;
  uint64_t new_ping;
  std::vector<CampaignInfo> new_campaigns;

  if (catalog.HasMember("catalogId")) {
    new_catalog_id = catalog["catalogId"].GetString();
  }

  if (catalog.HasMember("version")) {
    new_version = catalog["version"].GetUint64();

    if (new_version != 1) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'patch invalid', { reason: 'unsupported version', version: version }
      return false;
    }
  }

  if (catalog.HasMember("ping")) {
    new_ping = catalog["ping"].GetUint64();
  }

  if (!catalog.HasMember("campaigns")) {
    // TODO(Terry Mancey): Implement Log (#44)
    return false;
  }

  for (const auto& campaign : catalog["campaigns"].GetArray()) {
    if (!campaign.HasMember("campaignId")) {
      continue;
    }

    CampaignInfo campaign_info;

    campaign_info.campaign_id = campaign["campaignId"].GetString();
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog invalid', 'duplicated campaignId: ' + campaignId

    if (campaign.HasMember("name")) {
      campaign_info.name = campaign["name"].GetString();
    }

    if (campaign.HasMember("startAt")) {
      std::string start_at = campaign["startAt"].GetString();
      campaign_info.start_at = start_at;

      // TODO(Terry Mancey): Implement Log (#44)
      // 'Catalog invalid', 'invalid startAt for campaignId: ' + campaignId
    }

    if (campaign.HasMember("endAt")) {
      campaign_info.end_at = campaign["endAt"].GetString();

      // TODO(Terry Mancey): Implement Log (#44)
      // 'Catalog invalid', 'invalid endAt for campaignId: ' + campaignId
    }

    if (campaign.HasMember("dailyCap")) {
      campaign_info.daily_cap = campaign["dailyCap"].GetUint64();
    }

    if (campaign.HasMember("budget")) {
      campaign_info.budget = campaign["budget"].GetUint64();
    }

    if (campaign.HasMember("advertiserId")) {
      campaign_info.advertiser_id = campaign["advertiserId"].GetString();
    }

    // Creative sets
    if (campaign.HasMember("creativeSets")) {
      for (const auto& creative_set : campaign["creativeSets"].GetArray()) {
        if (!creative_set.HasMember("creativeSetId")) {
          continue;
        }

        CreativeSetInfo creative_set_info;

        creative_set_info.creative_set_id =
          creative_set["creativeSetId"].GetString();

        // TODO(Terry Mancey): Implement Log (#44)
        // 'Catalog invalid', 'duplicated creativeSetId: ' + creativeSetId

        if (creative_set.HasMember("execution")) {
          std::string execution = creative_set["execution"].GetString();

          if (execution != "per_click") {
            // TODO(Terry Mancey): Implement Log (#44)
            // 'Catalog invalid', 'creativeSet with unknown execution: '
            // + creativeSet.execution
            return false;
          }

          creative_set_info.execution = execution;
        }

        if (creative_set.HasMember("perDay")) {
          creative_set_info.per_day = creative_set["perDay"].GetUint64();
        }

        if (creative_set.HasMember("totalMax")) {
          creative_set_info.total_max = creative_set["totalMax"].GetUint64();
        }

        // Creatives
        if (creative_set.HasMember("creatives")) {
          for (const auto& creative : creative_set["creatives"].GetArray()) {
            if (!creative.HasMember("creativeId")) {
              continue;
            }

            CreativeInfo creative_info;

            creative_info.creative_instance_id =
              creative["creativeInstanceId"].GetString();

            // Type
            if (creative.HasMember("type")) {
              auto type = creative["type"].GetObject();

              if (type.HasMember("code")) {
                creative_info.type.code = type["code"].GetString();
              }

              if (type.HasMember("name")) {
                std::string name = type["name"].GetString();

                if (name != "notification") {
                  // TODO(Terry Mancey): Implement Log (#44)
                  // 'Catalog invalid', 'creative with invalid type: '
                  // + creative.creativeId + ' type=' + type
                  return false;
                }

                creative_info.type.name = name;
              }

              if (type.HasMember("platform")) {
                creative_info.type.platform = type["platform"].GetString();
              }

              if (type.HasMember("version")) {
                creative_info.type.version = type["version"].GetUint64();
              }
            }

            // Payload
            if (creative.HasMember("payload")) {
              auto payload = creative["payload"].GetObject();

              if (payload.HasMember("body")) {
                creative_info.payload.body = payload["body"].GetString();
              }

              if (payload.HasMember("title")) {
                creative_info.payload.title = payload["title"].GetString();
              }

              if (payload.HasMember("targetUrl")) {
                creative_info.payload.target_url =
                  payload["targetUrl"].GetString();
              }
            }

            creative_set_info.creatives.push_back(creative_info);
          }
        }

        // Segments
        if (creative_set.HasMember("segments")) {
          auto segments = creative_set["segments"].GetArray();
          if (segments.Size() == 0) {
            // TODO(Terry Mancey): Implement Log (#44)
            // 'Catalog invalid', 'creativeSet with no segments: '
            // + creativeSetId
            return false;
          }

          for (const auto& segment : segments) {
            SegmentInfo segment_info;

            if (segment.HasMember("code")) {
              segment_info.code = segment["code"].GetString();
            }

            if (segment.HasMember("name")) {
              segment_info.name = segment["name"].GetString();
            }

            creative_set_info.segments.push_back(segment_info);
          }
        }

        campaign_info.creative_sets.push_back(creative_set_info);
      }
    }

    // Geo targets
    if (campaign.HasMember("geoTargets")) {
      for (const auto& geo_target : campaign["geoTargets"].GetArray()) {
        GeoTargetInfo geo_target_info;

        if (geo_target.HasMember("code")) {
          geo_target_info.code = geo_target["code"].GetString();
        }

        if (geo_target.HasMember("name")) {
          geo_target_info.name = geo_target["name"].GetString();
        }

        campaign_info.geo_targets.push_back(geo_target_info);
      }
    }

    new_campaigns.push_back(campaign_info);
  }

  catalog_id = new_catalog_id;
  version = new_version;
  ping = new_ping;
  campaigns = new_campaigns;

  return true;
}

// TODO(Terry Mancey): Decouple validateJson by moving to json_helper class
bool CATALOG_STATE::validateJson(
    const rapidjson::Document& document,
    const std::map<std::string, std::string>& members) {
  for (const auto& member : document.GetObject()) {
    std::string member_name = member.name.GetString();
    std::string member_type = _rapidjson_member_types[member.value.GetType()];

    if (members.find(member_name) == members.end()) {
      // Member name not used
      continue;
    }

    std::string type = members.at(member_name);
    if (type != member_type) {
      // Invalid member type
      return false;
    }
  }

  return true;
}

}  // namespace ads

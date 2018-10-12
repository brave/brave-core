/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <map>

#include "../include/catalog_state.h"
#include "../include/json_helper.h"

namespace state {

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

bool CATALOG_STATE::LoadFromJson(const std::string& json) {
  rapidjson::Document catalog;
  catalog.Parse(json.c_str());

  if (catalog.HasParseError()) {
    LOG(ERROR) << "Failed to parse Catalog JSON" << std::endl;

    return false;
  }

  const std::map<std::string, std::string> members = {
    {"catalogId", "String"},
    {"version", "Number"},
    {"ping", "Number"},
    {"campaigns", "Array"}
  };

  // TODO(Terry Mancey): Refactor validateJson by moving to json_helper class
  validateJson(catalog, members);

  std::string new_catalog_id;
  uint64_t new_version;
  uint64_t new_ping;
  std::vector<catalog::CampaignInfo> new_campaigns;

  if (catalog.HasMember("catalogId")) {
    new_catalog_id = catalog["catalogId"].GetString();
  }

  if (catalog.HasMember("version")) {
    new_version = catalog["version"].GetUint64();

    if (new_version != 1) {
      // TODO(Terry Mancey): Implement User Model Log (#44)
      return false;
    }
  }

  if (catalog.HasMember("ping")) {
    new_ping = catalog["ping"].GetUint64();
  }

  if (!catalog.HasMember("campaigns")) {
    // TODO(Terry Mancey): Implement User Model Log (#44)
    return false;
  }

  for (auto& campaign : catalog["campaigns"].GetArray()) {
    if (!campaign.HasMember("campaignId")) {
      LOG(WARNING) << "campaignId missing from Catalog JSON" << std::endl;
      continue;
    }

    catalog::CampaignInfo campaign_info;

    campaign_info.campaign_id = campaign["campaignId"].GetString();

    if (campaign.HasMember("name")) {
      campaign_info.name = campaign["name"].GetString();
    }

    if (campaign.HasMember("startAt")) {
      std::string start_at = campaign["startAt"].GetString();
      campaign_info.start_at = start_at;
    }

    if (campaign.HasMember("endAt")) {
      campaign_info.end_at = campaign["endAt"].GetString();
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
      for (auto& creative_set : campaign["creativeSets"].GetArray()) {
        if (!creative_set.HasMember("creativeSetId")) {
          continue;
        }

        catalog::CreativeSetInfo creative_set_info;

        creative_set_info.creative_set_id =
          creative_set["creativeSetId"].GetString();

        if (creative_set.HasMember("execution")) {
          std::string execution = creative_set["execution"].GetString();

          if (execution.compare("per_click") != 0) {
            // TODO(Terry Mancey): Implement User Model Log (#44)
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
          for (auto& creative : creative_set["creatives"].GetArray()) {
            if (!creative.HasMember("creativeId")) {
              LOG(WARNING) << "creativeId missing from Catalog JSON"
                << std::endl;
              continue;
            }

            catalog::CreativeInfo creative_info;

            creative_info.creative_id = creative["creativeId"].GetString();

            // Type
            if (creative.HasMember("type")) {
              auto type = creative["type"].GetObject();

              if (type.HasMember("code")) {
                creative_info.type.code = type["code"].GetString();
              }

              if (type.HasMember("name")) {
                std::string name = type["name"].GetString();

                if (name.compare("notification") != 0) {
                // TODO(Terry Mancey): Implement User Model Log (#44)
                  return false;
                }

                creative_info.type.code = name;
              }

              if (type.HasMember("platform")) {
                creative_info.type.code = type["platform"].GetString();
              }

              if (type.HasMember("version")) {
                creative_info.type.code = type["version"].GetUint64();
              }
            }

            // Payload
            if (creative.HasMember("payload")) {
              auto payload = creative["payload"].GetObject();

              if (payload.HasMember("body")) {
                creative_info.type.code = payload["body"].GetString();
              }

              if (payload.HasMember("title")) {
                creative_info.type.code = payload["title"].GetString();
              }

              if (payload.HasMember("targetUrl")) {
                creative_info.type.code = payload["targetUrl"].GetString();
              }
            }

            creative_set_info.creatives.push_back(creative_info);
          }
        }

        // Segments
        if (creative_set.HasMember("segments")) {
          auto segments = creative_set["segments"].GetArray();
          if (segments.Size() == 0) {
            // TODO(Terry Mancey): Implement User Model Log (#44)
            return false;
          }

          for (auto& segment : segments) {
            catalog::SegmentInfo segment_info;

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
      for (auto& geo_target : campaign["geoTargets"].GetArray()) {
        catalog::GeoTargetInfo geo_target_info;

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

// TODO(Terry Mancey): Refactor validateJson by moving to json_helper class
bool CATALOG_STATE::validateJson(
    const rapidjson::Document& document,
    const std::map<std::string, std::string>& members) {
  for (auto& member : document.GetObject()) {
    std::string member_name = member.name.GetString();
    std::string member_type = _rapidjson_member_types[member.value.GetType()];

    if (members.find(member_name) == members.end()) {
      LOG(WARNING) "JSON " << member_name << " member not used" << std::endl;
      continue;
    }

    std::string type = members.at(member_name);
    if (type.compare(member_type) != 0) {
      LOG(WARNING) << "Invalid type for JSON member "
        << member_name << std::endl;
      return false;
    }
  }

  return false;
}

void SaveToJson(JsonWriter& writer, const CATALOG_STATE& state) {
  writer.StartObject();

  writer.String("catalog_id");
  writer.String(state.catalog_id.c_str());

  writer.String("version");
  writer.Uint64(state.version);

  writer.String("ping");
  writer.Uint64(state.ping);

  writer.String("campaigns");
  writer.StartArray();
  for (auto& campaign : state.campaigns) {
  }
  writer.EndArray();

  writer.EndObject();
}

}  // namespace state

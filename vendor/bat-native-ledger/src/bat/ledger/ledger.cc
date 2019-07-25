/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/media/media.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/ledger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace ledger {

bool is_production = true;
bool is_debug = false;
bool is_testing = false;
int reconcile_time = 0;  // minutes
bool short_retries = false;

ActivityInfoFilter::ActivityInfoFilter() :
    excluded(EXCLUDE_FILTER::FILTER_DEFAULT),
    percent(0),
    min_duration(0),
    reconcile_stamp(0),
    non_verified(true),
    min_visits(0u) {}

ActivityInfoFilter::ActivityInfoFilter(const ActivityInfoFilter& filter) :
    id(filter.id),
    excluded(filter.excluded),
    percent(filter.percent),
    order_by(filter.order_by),
    min_duration(filter.min_duration),
    reconcile_stamp(filter.reconcile_stamp),
    non_verified(filter.non_verified),
    min_visits(filter.min_visits) {}

ActivityInfoFilter::~ActivityInfoFilter() {}

const std::string ActivityInfoFilter::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool ActivityInfoFilter::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("id") && d["id"].IsString() &&
        d.HasMember("excluded") && d["excluded"].IsInt() &&
        d.HasMember("percent") && d["percent"].IsUint() &&
        d.HasMember("order_by") && d["order_by"].IsObject() &&
        d.HasMember("min_duration") && d["min_duration"].IsUint64() &&
        d.HasMember("reconcile_stamp") && d["reconcile_stamp"].IsUint64() &&
        d.HasMember("non_verified") && d["non_verified"].IsBool());
  }

  if (!error) {
    id = d["id"].GetString();
    excluded = (EXCLUDE_FILTER)d["excluded"].GetInt();
    percent = d["percent"].GetUint();
    min_duration = d["min_duration"].GetUint64();
    reconcile_stamp = d["reconcile_stamp"].GetUint64();
    non_verified = d["non_verified"].GetBool();

    for (const auto& i : d["order_by"].GetObject()) {
      order_by.push_back(std::make_pair(i.name.GetString(),
            i.value.GetBool()));
    }

    if (d.HasMember("min_visits") && d["min_visits"].IsUint()) {
      min_visits = d["min_visits"].GetUint();
    } else {
      min_visits = 0u;
    }
  }

  return !error;
}

// static
ledger::Ledger* Ledger::CreateInstance(LedgerClient* client) {
  return new bat_ledger::LedgerImpl(client);
}

ReconcileInfo::ReconcileInfo()
    : retry_step_(ContributionRetry::STEP_NO), retry_level_(0) {}
ReconcileInfo::~ReconcileInfo() {}
ReconcileInfo::ReconcileInfo(const ledger::ReconcileInfo& info) {
  viewingId_ = info.viewingId_;
  amount_ = info.amount_;
  retry_step_ = info.retry_step_;
  retry_level_ = info.retry_level_;
}

const std::string ReconcileInfo::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool ReconcileInfo::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (false == error) {
    error =
        !(d.HasMember("viewingId") && d["viewingId"].IsString() &&
          d.HasMember("amount") && d["amount"].IsString() &&
          d.HasMember("retry_step") && d["retry_step"].IsInt() &&
          d.HasMember("retry_level") && d["retry_level"].IsInt());
  }

  if (false == error) {
    viewingId_ = d["viewingId"].GetString();
    amount_ = d["amount"].GetString();
    retry_step_ = static_cast<ContributionRetry>(d["retry_step"].GetInt());
    retry_level_ = d["retry_level"].GetInt();
  }

  return !error;
}

RewardsInternalsInfo::RewardsInternalsInfo() {}

RewardsInternalsInfo::RewardsInternalsInfo(const RewardsInternalsInfo& info)
    : payment_id(info.payment_id),
      is_key_info_seed_valid(info.is_key_info_seed_valid),
      current_reconciles(info.current_reconciles) {}

RewardsInternalsInfo::~RewardsInternalsInfo() {}

const std::string RewardsInternalsInfo::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool RewardsInternalsInfo::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (false == error) {
    error = !(d.HasMember("payment_id") && d["payment_id"].IsString() &&
              d.HasMember("is_key_info_seed_valid") &&
              d["is_key_info_seed_valid"].IsBool() &&
              d.HasMember("current_reconciles") &&
              d["current_reconciles"].IsArray());
  }

  if (false == error) {
    payment_id = d["payment_id"].GetString();
    is_key_info_seed_valid = d["is_key_info_seed_valid"].GetBool();

    for (const auto& i : d["current_reconciles"].GetArray()) {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      i.Accept(writer);

      ReconcileInfo reconcile_info;
      reconcile_info.loadFromJson(sb.GetString());
      current_reconciles.insert(
          std::make_pair(reconcile_info.viewingId_, reconcile_info));
    }

    persona_id = d["persona_id"].GetString();
    user_id = d["user_id"].GetString();
    boot_stamp = d["boot_stamp"].GetUint64();
  }

  return !error;
}

bool Ledger::IsMediaLink(const std::string& url,
                         const std::string& first_party_url,
                         const std::string& referrer) {
  const std::string type = braveledger_media::Media::GetLinkType(
      url,
      first_party_url,
      referrer);

  return type == TWITCH_MEDIA_TYPE || type == VIMEO_MEDIA_TYPE;
}

}  // namespace ledger

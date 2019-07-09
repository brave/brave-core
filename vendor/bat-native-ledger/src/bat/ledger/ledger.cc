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

MediaEventInfo::MediaEventInfo() {}

MediaEventInfo::MediaEventInfo(const MediaEventInfo& info):
  event_(info.event_),
  time_(info.time_),
  status_(info.status_) {}

MediaEventInfo::~MediaEventInfo() {}


// static
ledger::Ledger* Ledger::CreateInstance(LedgerClient* client) {
  return new bat_ledger::LedgerImpl(client);
}

BalanceReportInfo::BalanceReportInfo():
  opening_balance_("0"),
  closing_balance_("0"),
  grants_("0"),
  earning_from_ads_("0"),
  auto_contribute_("0"),
  recurring_donation_("0"),
  one_time_donation_("0"),
  total_("0") {}

BalanceReportInfo::BalanceReportInfo(const BalanceReportInfo& state) {
  opening_balance_ = state.opening_balance_;
  closing_balance_ = state.closing_balance_;
  deposits_ = state.deposits_;
  grants_ = state.grants_;
  earning_from_ads_ = state.earning_from_ads_;
  auto_contribute_ = state.auto_contribute_;
  recurring_donation_ = state.recurring_donation_;
  one_time_donation_ = state.one_time_donation_;
  total_ = state.total_;
}

BalanceReportInfo::~BalanceReportInfo() {}

const std::string BalanceReportInfo::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool BalanceReportInfo::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(
        d.HasMember("opening_balance_") && d["opening_balance_"].IsString() &&
        d.HasMember("closing_balance_") && d["closing_balance_"].IsString() &&
        d.HasMember("deposits_") && d["deposits_"].IsString() &&
        d.HasMember("grants_") && d["grants_"].IsString() &&
        d.HasMember("earning_from_ads_") &&
        d["earning_from_ads_"].IsString() &&
        d.HasMember("auto_contribute_") && d["auto_contribute_"].IsString() &&
        d.HasMember("recurring_donation_") &&
        d["recurring_donation_"].IsString() &&
        d.HasMember("one_time_donation_") &&
        d["one_time_donation_"].IsString() &&
        d.HasMember("total_") && d["total_"].IsString());
  }

  if (!error) {
    opening_balance_ = d["opening_balance_"].GetString();
    closing_balance_ = d["closing_balance_"].GetString();
    deposits_ = d["deposits_"].GetString();
    grants_ = d["grants_"].GetString();
    earning_from_ads_ = d["earning_from_ads_"].GetString();
    auto_contribute_ = d["auto_contribute_"].GetString();
    recurring_donation_ = d["recurring_donation_"].GetString();
    one_time_donation_ = d["one_time_donation_"].GetString();
    total_ = d["total_"].GetString();
  }

  return !error;
}

AutoContributeProps::AutoContributeProps()
  : enabled_contribute(false),
    contribution_min_time(0),
    contribution_min_visits(0),
    contribution_non_verified(false),
    contribution_videos(false),
    reconcile_stamp(0) { }

AutoContributeProps::~AutoContributeProps() { }

const std::string AutoContributeProps::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool AutoContributeProps::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("enabled_contribute") &&
        d["enabled_contribute"].IsBool() &&
        d.HasMember("contribution_min_time") &&
        d["contribution_min_time"].IsUint64() &&
        d.HasMember("contribution_min_visits") &&
        d["contribution_min_visits"].IsInt() &&
        d.HasMember("contribution_non_verified") &&
        d["contribution_non_verified"].IsBool() &&
        d.HasMember("contribution_videos") &&
        d["contribution_videos"].IsBool() &&
        d.HasMember("reconcile_stamp") && d["reconcile_stamp"].IsUint64());
  }

  if (!error) {
    enabled_contribute = d["enabled_contribute"].GetBool();
    contribution_min_time = d["contribution_min_time"].GetUint64();
    contribution_min_visits = d["contribution_min_visits"].GetInt();
    contribution_non_verified = d["contribution_non_verified"].GetBool();
    contribution_videos = d["contribution_videos"].GetBool();
    reconcile_stamp = d["reconcile_stamp"].GetUint64();
  }

  return !error;
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

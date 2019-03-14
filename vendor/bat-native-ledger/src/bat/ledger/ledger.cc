/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bat_get_media.h"
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

VisitData::VisitData():
    tab_id(-1) {}

VisitData::VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id,
            const std::string& _name,
            const std::string& _url,
            const std::string& _provider,
            const std::string& _favicon_url) :
    tld(_tld),
    domain(_domain),
    path(_path),
    tab_id(_tab_id),
    name(_name),
    url(_url),
    provider(_provider),
    favicon_url(_favicon_url) {}

VisitData::VisitData(const VisitData& data) :
    tld(data.tld),
    domain(data.domain),
    path(data.path),
    tab_id(data.tab_id),
    name(data.name),
    url(data.url),
    provider(data.provider),
    favicon_url(data.favicon_url) {}

VisitData::~VisitData() {}

const std::string VisitData::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool VisitData::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("tld") && d["tld"].IsString() &&
        d.HasMember("domain") && d["domain"].IsString() &&
        d.HasMember("path") && d["path"].IsString() &&
        d.HasMember("tab_id") && d["tab_id"].IsUint() &&
        d.HasMember("name") && d["name"].IsString() &&
        d.HasMember("url") && d["url"].IsString() &&
        d.HasMember("provider") && d["provider"].IsString() &&
        d.HasMember("favicon_url") && d["favicon_url"].IsString());
  }

  if (!error) {
    tld = d["tld"].GetString();
    domain = d["domain"].GetString();
    path = d["path"].GetString();
    tab_id = d["tab_id"].GetUint();
    name = d["name"].GetString();
    url = d["url"].GetString();
    provider = d["provider"].GetString();
    favicon_url = d["favicon_url"].GetString();
  }

  return !error;
}


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

PublisherBanner::PublisherBanner() {}

PublisherBanner::PublisherBanner(const PublisherBanner& info) :
    publisher_key(info.publisher_key),
    title(info.title),
    name(info.name),
    description(info.description),
    background(info.background),
    logo(info.logo),
    amounts(info.amounts),
    provider(info.provider),
    social(info.social),
    verified(info.verified) {}

PublisherBanner::~PublisherBanner() {}

const std::string PublisherBanner::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool PublisherBanner::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());
  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("publisher_key") && d["publisher_key"].IsString() &&
        d.HasMember("title") && d["title"].IsString() &&
        d.HasMember("name") && d["name"].IsString() &&
        d.HasMember("description") && d["description"].IsString() &&
        d.HasMember("background") && d["name"].IsString() &&
        d.HasMember("logo") && d["logo"].IsString() &&
        d.HasMember("amounts") && d["amounts"].IsArray() &&
        d.HasMember("social") && d["social"].IsObject() &&
        d.HasMember("verified") && d["verified"].IsBool());
  }

  if (!error) {
    publisher_key = d["publisher_key"].GetString();
    title = d["title"].GetString();
    name = d["name"].GetString();
    description = d["description"].GetString();
    background = d["background"].GetString();
    logo = d["logo"].GetString();
    verified = d["verified"].GetBool();

    for (const auto& amount : d["amounts"].GetArray()) {
      amounts.push_back(amount.GetInt());
    }

    for (const auto& i : d["social"].GetObject()) {
      social.insert(std::make_pair(i.name.GetString(),
            i.value.GetString()));
    }

    if (d.HasMember("provider")) {
      provider = d["provider"].GetString();
    }
  }

  return !error;
}

PublisherInfo::PublisherInfo() :
    duration(0u),
    score(.0),
    visits(0u),
    percent(0u),
    weight(.0),
    excluded(PUBLISHER_EXCLUDE::DEFAULT),
    category(REWARDS_CATEGORY::AUTO_CONTRIBUTE),
    reconcile_stamp(0),
    verified(false),
    name(""),
    url(""),
    provider(""),
    favicon_url("") {}

PublisherInfo::PublisherInfo(const std::string& publisher_id) :
    id(publisher_id),
    duration(0u),
    score(.0),
    visits(0u),
    percent(0u),
    weight(.0),
    excluded(PUBLISHER_EXCLUDE::DEFAULT),
    category(REWARDS_CATEGORY::AUTO_CONTRIBUTE),
    reconcile_stamp(0),
    verified(false),
    name(""),
    url(""),
    provider(""),
    favicon_url("") {}

PublisherInfo::PublisherInfo(const PublisherInfo& info) :
    id(info.id),
    duration(info.duration),
    score(info.score),
    visits(info.visits),
    percent(info.percent),
    weight(info.weight),
    excluded(info.excluded),
    category(info.category),
    reconcile_stamp(info.reconcile_stamp),
    verified(info.verified),
    name(info.name),
    url(info.url),
    provider(info.provider),
    favicon_url(info.favicon_url),
    contributions(info.contributions) {}

PublisherInfo::~PublisherInfo() {}

bool PublisherInfo::operator<(const PublisherInfo& rhs) const {
  return score > rhs.score;
}

bool PublisherInfo::is_valid() const {
  return !id.empty();
}

const std::string PublisherInfo::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool PublisherInfo::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());
  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("id") && d["id"].IsString() &&
        d.HasMember("duration") && d["duration"].IsUint64() &&
        d.HasMember("score") && d["score"].IsDouble() &&
        d.HasMember("visits") && d["visits"].IsUint() &&
        d.HasMember("percent") && d["percent"].IsUint() &&
        d.HasMember("weight") && d["weight"].IsDouble() &&
        d.HasMember("excluded") && d["excluded"].IsInt() &&
        d.HasMember("category") && d["category"].IsInt() &&
        d.HasMember("reconcile_stamp") && d["reconcile_stamp"].IsUint64() &&
        d.HasMember("verified") && d["verified"].IsBool() &&
        d.HasMember("name") && d["name"].IsString() &&
        d.HasMember("url") && d["url"].IsString() &&
        d.HasMember("provider") && d["provider"].IsString() &&
        d.HasMember("favicon_url") && d["favicon_url"].IsString() &&
        d.HasMember("contributions") && d["contributions"].IsArray());
  }

  if (!error) {
    id = d["id"].GetString();
    duration = d["duration"].GetUint64();
    score = d["score"].GetDouble();
    visits = d["visits"].GetUint();
    percent = d["percent"].GetUint();
    weight = d["weight"].GetDouble();
    excluded = (PUBLISHER_EXCLUDE)d["excluded"].GetInt();
    category = (REWARDS_CATEGORY)d["category"].GetInt();
    reconcile_stamp = d["reconcile_stamp"].GetUint64();
    verified = d["verified"].GetBool();
    name = d["name"].GetString();
    url = d["url"].GetString();
    provider = d["provider"].GetString();
    favicon_url = d["favicon_url"].GetString();

    for (const auto& contribution : d["contributions"].GetArray()) {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      contribution.Accept(writer);

      ContributionInfo info;
      info.loadFromJson(sb.GetString());
      contributions.push_back(info);
    }
  }

  return !error;
}

const PublisherInfo invalid("");

const std::string ContributionInfo::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool ContributionInfo::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("publisher") && d["publisher"].IsString() &&
        d.HasMember("value") && d["value"].IsDouble() &&
        d.HasMember("date") && d["date"].IsUint64());
  }

  if (!error) {
    publisher = d["publisher"].GetString();
    value = d["value"].GetDouble();
    date = d["date"].GetUint64();
  }

  return !error;
}

TwitchEventInfo::TwitchEventInfo() {}

TwitchEventInfo::TwitchEventInfo(const TwitchEventInfo& info):
  event_(info.event_),
  time_(info.time_),
  status_(info.status_) {}

TwitchEventInfo::~TwitchEventInfo() {}


// static
ledger::Ledger* Ledger::CreateInstance(LedgerClient* client) {
  return new bat_ledger::LedgerImpl(client);
}

WalletInfo::WalletInfo() : balance_(0), fee_amount_(0), parameters_days_(0) {}
WalletInfo::~WalletInfo() {}
WalletInfo::WalletInfo(const ledger::WalletInfo &info) {
  altcurrency_ = info.altcurrency_;
  probi_ = info.probi_;
  balance_ = info.balance_;
  fee_amount_ = info.fee_amount_;
  rates_ = info.rates_;
  parameters_choices_ = info.parameters_choices_;
  parameters_range_ = info.parameters_range_;
  parameters_days_ = info.parameters_days_;
  grants_ = info.grants_;
}

const std::string WalletInfo::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool WalletInfo::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("altcurrency_") && d["altcurrency_"].IsString() &&
        d.HasMember("probi_") && d["probi_"].IsString() &&
        d.HasMember("balance_") && d["balance_"].IsDouble() &&
        d.HasMember("fee_amount_") && d["fee_amount_"].IsDouble() &&
        d.HasMember("rates_") && d["rates_"].IsObject() &&
        d.HasMember("parameters_choices_") &&
        d["parameters_choices_"].IsArray() &&
        d.HasMember("parameters_range_") && d["parameters_range_"].IsArray() &&
        d.HasMember("parameters_days_") && d["parameters_days_"].IsUint() &&
        d.HasMember("grants_") && d["grants_"].IsArray());
  }

  if (!error) {
    altcurrency_ = d["altcurrency_"].GetString();
    probi_ = d["probi_"].GetString();
    balance_ = d["balance_"].GetDouble();
    fee_amount_ = d["fee_amount_"].GetDouble();
    parameters_days_ = d["parameters_days_"].GetUint();

    for (const auto& rate : d["rates_"].GetObject()) {
      rates_.insert(std::make_pair(rate.name.GetString(),
            rate.value.GetDouble()));
    }

    for (const auto& parameters_choice : d["parameters_choices_"].GetArray()) {
      parameters_choices_.push_back(parameters_choice.GetDouble());
    }

    for (const auto& parameters_choice : d["parameters_range_"].GetArray()) {
      parameters_range_.push_back(parameters_choice.GetDouble());
    }

    for (const auto& g : d["grants_"].GetArray()) {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      g.Accept(writer);

      Grant grant;
      grant.loadFromJson(sb.GetString());
      grants_.push_back(grant);
    }
  }

  return !error;
}

Grant::Grant() {}
Grant::~Grant() {}
Grant::Grant(const ledger::Grant &properties) {
  promotionId = properties.promotionId;
  expiryTime = properties.expiryTime;
  probi = properties.probi;
  altcurrency = properties.altcurrency;
  type = properties.type;
}

const std::string Grant::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool Grant::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("altcurrency") && d["altcurrency"].IsString() &&
        d.HasMember("probi") && d["probi"].IsString() &&
        d.HasMember("promotionId") && d["promotionId"].IsString() &&
        d.HasMember("expiryTime") && d["expiryTime"].IsUint64());
  }

  if (!error) {
    altcurrency = d["altcurrency"].GetString();
    probi = d["probi"].GetString();
    promotionId = d["promotionId"].GetString();
    expiryTime = d["expiryTime"].GetUint64();

    // Check for type attribute, old grants which
    // do not have the type attribute should default to ugp
    if (d.HasMember("type") && d["type"].IsString()) {
      type = d["type"].GetString();
    } else {
      type = "ugp";
    }
  }

  return !error;
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
  }

  return !error;
}

bool Ledger::IsMediaLink(const std::string& url,
                         const std::string& first_party_url,
                         const std::string& referrer) {
  const std::string type = braveledger_bat_get_media::BatGetMedia::GetLinkType(
      url,
      first_party_url,
      referrer);

  return type == TWITCH_MEDIA_TYPE;
}

PendingContribution::PendingContribution() {}
PendingContribution::~PendingContribution() {}
PendingContribution::PendingContribution(
    const ledger::PendingContribution &properties) {
  publisher_key = properties.publisher_key;
  amount = properties.amount;
  added_date = properties.added_date;
  viewing_id = properties.viewing_id;
  category = properties.category;
}

const std::string PendingContribution::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool PendingContribution::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("publisher_key") && d["publisher_key"].IsString() &&
        d.HasMember("amount") && d["amount"].IsDouble() &&
        d.HasMember("added_date") && d["added_date"].IsUint64() &&
        d.HasMember("viewing_id") && d["viewing_id"].IsString() &&
        d.HasMember("category") && d["category"].IsInt());
  }

  if (!error) {
    publisher_key = d["publisher_key"].GetString();
    amount = d["amount"].GetDouble();
    added_date = d["added_date"].GetUint64();
    viewing_id = d["viewing_id"].GetString();
    category = static_cast<REWARDS_CATEGORY>(d["category"].GetInt());
  }

  return !error;
}

PendingContributionList::PendingContributionList() {}
PendingContributionList::~PendingContributionList() {}
PendingContributionList::PendingContributionList(
    const ledger::PendingContributionList &properties) {
  list_ = properties.list_;
}

const std::string PendingContributionList::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool PendingContributionList::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("list") && d["list"].IsArray());
  }

  if (!error) {
    for (const auto& g : d["list"].GetArray()) {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      g.Accept(writer);

      PendingContribution contribution;
      contribution.loadFromJson(sb.GetString());
      list_.push_back(contribution);
    }
  }

  return !error;
}

PublisherInfoListStruct::PublisherInfoListStruct() {}
PublisherInfoListStruct::~PublisherInfoListStruct() {}
PublisherInfoListStruct::PublisherInfoListStruct(
    const ledger::PublisherInfoListStruct &properties) {
  list = properties.list;
}

const std::string PublisherInfoListStruct::ToJson() const {
  std::string json;
  braveledger_bat_helper::saveToJsonString(*this, &json);
  return json;
}

bool PublisherInfoListStruct::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();

  if (!error) {
    error = !(d.HasMember("list") && d["list"].IsArray());
  }

  if (!error) {
    for (const auto& g : d["list"].GetArray()) {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      g.Accept(writer);

      PublisherInfo contribution;
      contribution.loadFromJson(sb.GetString());
      list.push_back(contribution);
    }
  }

  return !error;
}

}  // namespace ledger

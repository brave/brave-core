/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger.h"

#include "ledger_impl.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace ledger {

VisitData::VisitData():
    tab_id(-1) {}

VisitData::VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id,
            PUBLISHER_MONTH _local_month,
            const std::string& _local_year) :
    tld(_tld),
    domain(_domain),
    path(_path),
    tab_id(_tab_id),
    local_month(_local_month),
    local_year(_local_year) {}

VisitData::VisitData(const VisitData& data) :
    tld(data.tld),
    domain(data.domain),
    path(data.path),
    tab_id(data.tab_id),
    local_month(data.local_month),
    local_year(data.local_year) {}

VisitData::~VisitData() {}


PaymentData::PaymentData():
  value(0),
  timestamp(0),
  category(PUBLISHER_CATEGORY::TIPPING) {}

PaymentData::PaymentData(const std::string& _domain,
         const double& _value,
         const int64_t& _timestamp,
         PUBLISHER_CATEGORY _category,
         PUBLISHER_MONTH _local_month,
         const std::string& _local_year):
  domain(_domain),
  value(_value),
  timestamp(_timestamp),
  category(_category),
  local_month(_local_month),
  local_year(_local_year) {}

PaymentData::PaymentData(const PaymentData& data):
  domain(data.domain),
  value(data.value),
  timestamp(data.timestamp),
  category(data.category),
  local_month(data.local_month),
  local_year(data.local_year) {}

PaymentData::~PaymentData() {}


PublisherInfoFilter::PublisherInfoFilter(int category_, 
    PUBLISHER_MONTH month_, const std::string& year_):
  category(PUBLISHER_CATEGORY::ALL_CATEGORIES),
  month(month_),
  year(year_) {}


const PublisherInfo invalid("");


PublisherInfo::PublisherInfo(const id_type& publisher_id) :
    id(publisher_id),
    duration(0u),
    score(.0),
    visits(0u),
    pinned(false),
    percent(0u),
    weight(.0),
    excluded(false),
    category(PUBLISHER_CATEGORY::AUTO_CONTRIBUTE) {}

PublisherInfo::PublisherInfo(const PublisherInfo& info) :
    id(info.id),
    duration(info.duration),
    score(info.score),
    visits(info.visits),
    pinned(info.pinned),
    percent(info.percent),
    weight(info.weight),
    excluded(info.excluded),
    key(info.key),
    contributions(info.contributions),
    category(info.category),
    month(info.month),
    year(info.year) {}

PublisherInfo::~PublisherInfo() {}

bool PublisherInfo::is_valid() const {
  return !id.empty();
}

const std::string PublisherInfo::ToJSON() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("id");
  writer.String(id.c_str());

  writer.String("duration");
  writer.Uint(duration);

  writer.String("score");
  writer.Double(score);

  writer.String("visits");
  writer.Uint(visits);

  writer.String("pinned");
  writer.Bool(pinned);

  writer.String("percent");
  writer.Uint(percent);

  writer.String("weight");
  writer.Double(weight);

  writer.String("excluded");
  writer.Bool(excluded);

  writer.String("key");
  writer.String(key.c_str());

  writer.String("contributions");
  writer.StartArray();
  for (size_t i = 0; i < contributions.size(); i++) {
    writer.StartObject();
    writer.String("publisher");
    writer.String(contributions[i].publisher.c_str());
    writer.String("value");
    writer.Double(contributions[i].value);
    writer.String("date");
    writer.Uint64(contributions[i].date);
    writer.EndObject();
  }
  writer.EndArray();

  writer.String("category");
  writer.Int(category);

  writer.String("month");
  writer.Int(month);

  writer.String("year");
  writer.String(year.c_str());

  writer.EndObject();

  return buffer.GetString();
}

/*bool PublisherInfo::Matches(PublisherInfoFilter filter) const {
  if (filter == PublisherInfoFilter::ALL)
    return true;

  if ((filter & PublisherInfoFilter::UNPINNED) == 0 && !pinned)
    return false;

  if ((filter & PublisherInfoFilter::PINNED) == 0 && pinned)
    return false;

  if ((filter & PublisherInfoFilter::INCLUDED) == 0 && excluded)
    return false;

  return true;
}*/

// static
const PublisherInfo PublisherInfo::FromJSON(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  if (d.HasParseError() ||
      !d["id"].IsString() ||
      !d["duration"].IsUint() ||
      !d["score"].IsDouble() ||
      !d["visits"].IsUint() ||
      !d["pinned"].IsBool() ||
      !d["percent"].IsUint() ||
      !d["weight"].IsDouble() ||
      !d["excluded"].IsBool() ||
      !d["key"].IsString() ||
      !d["contributions"].IsArray() ||
      !d["category"].IsInt() ||
      !d["month"].IsInt() ||
      !d["year"].IsString()) {
    return invalid;
  }

  PublisherInfo info(d["id"].GetString());
  info.duration = d["duration"].GetUint();
  info.score = d["score"].GetDouble();
  info.visits = d["visits"].GetUint();
  info.pinned = d["pinned"].GetBool();
  info.percent = d["percent"].GetUint();
  info.weight = d["weight"].GetDouble();
  info.excluded = d["excluded"].GetBool();
  info.key = d["key"].GetString();

  for (const auto & i : d["contributions"].GetArray()) {
    ContributionInfo contribution_info;
    auto obj = i.GetObject();
    if (obj.HasMember("value") && obj["value"].IsDouble()) {
      contribution_info.value = obj["value"].GetDouble();
    }
    if (obj.HasMember("date") && obj["date"].IsUint64()) {
      contribution_info.date = obj["date"].GetUint64();
    }
    if (obj.HasMember("publisher") && obj["publisher"].IsString()) {
      contribution_info.publisher = obj["publisher"].GetString();
    }
    info.contributions.push_back(contribution_info);
  }

  info.category = (PUBLISHER_CATEGORY)d["category"].GetInt();
  info.month = (PUBLISHER_MONTH)d["month"].GetInt();
  info.year = d["year"].GetString();

  return info;
}

// static
ledger::Ledger* Ledger::CreateInstance(LedgerClient* client) {
  return new bat_ledger::LedgerImpl(client);
}

WalletInfo::WalletInfo () : balance_(0), parameters_days_(0) {}
WalletInfo::~WalletInfo () {}
WalletInfo::WalletInfo (const ledger::WalletInfo &info) {
  altcurrency_ = info.altcurrency_;
  probi_ = info.probi_;
  balance_ = info.balance_;
  rates_ = info.rates_;
  parameters_choices_ = info.parameters_choices_;
  parameters_range_ = info.parameters_range_;
  parameters_days_ = info.parameters_days_;
  grants_ = info.grants_;
}

Promo::Promo () : amount(0) {}
Promo::~Promo () {}
Promo::Promo (const ledger::Promo &properties) {
  promotionId = properties.promotionId;
  amount = properties.amount;
}

BalanceReportInfo::BalanceReportInfo():
  opening_balance_(.0),
  closing_balance_(.0),
  grants_(.0),
  earning_from_ads_(.0),
  auto_contribute_(.0),
  recurring_donation_(.0),
  one_time_donation_(.0) {}
}

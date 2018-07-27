/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger.h"

#include "ledger_impl.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace ledger {

VisitData::VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id) :
    tld(_tld),
    domain(_domain),
    path(_path),
    tab_id(_tab_id) {}

VisitData::VisitData(const VisitData& data) :
    tld(data.tld),
    domain(data.domain),
    path(data.path),
    tab_id(data.tab_id),
    duration(data.duration) {}

const PublisherInfo invalid("");

PublisherInfo::PublisherInfo(const id_type& publisher_id) :
    id(publisher_id),
    duration(0u),
    score(.0),
    visits(0u),
    pinned(false),
    percent(0u),
    weight(.0),
    excluded(false) {}

PublisherInfo::PublisherInfo(const PublisherInfo& info) :
    id(info.id),
    duration(info.duration),
    score(info.score),
    visits(info.visits),
    pinned(info.pinned),
    percent(info.percent),
    weight(info.weight),
    excluded(info.excluded) {}

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

  writer.EndObject();

  return buffer.GetString();
}

bool PublisherInfo::Matches(PublisherInfoFilter filter) const {
  if (filter == PublisherInfoFilter::ALL)
    return true;

  if ((filter & PublisherInfoFilter::UNPINNED) == 0 && !pinned)
    return false;

  if ((filter & PublisherInfoFilter::PINNED) == 0 && pinned)
    return false;

  if ((filter & PublisherInfoFilter::INCLUDED) == 0 && excluded)
    return false;

  return true;
}

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
      !d["excluded"].IsBool()) {
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
}

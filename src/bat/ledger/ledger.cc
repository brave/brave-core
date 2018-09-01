/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger.h"

#include "ledger_impl.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "bat_get_media.h"

namespace ledger {

VisitData::VisitData():
    tab_id(-1) {}

VisitData::VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id,
            PUBLISHER_MONTH _local_month,
            int _local_year) :
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
    local_year(data.local_year),
    favIconURL(data.favIconURL) {}

VisitData::~VisitData() {}


PaymentData::PaymentData():
  value(0),
  timestamp(0),
  category(PUBLISHER_CATEGORY::TIPPING) {}

PaymentData::PaymentData(const std::string& _publisher_id,
         const double& _value,
         const int64_t& _timestamp,
         PUBLISHER_CATEGORY _category,
         PUBLISHER_MONTH _local_month,
         int _local_year):
  publisher_id(_publisher_id),
  value(_value),
  timestamp(_timestamp),
  category(_category),
  local_month(_local_month),
  local_year(_local_year) {}

PaymentData::PaymentData(const PaymentData& data):
  publisher_id(data.publisher_id),
  value(data.value),
  timestamp(data.timestamp),
  category(data.category),
  local_month(data.local_month),
  local_year(data.local_year) {}

PaymentData::~PaymentData() {}

PublisherInfoFilter::PublisherInfoFilter() :
    category(PUBLISHER_CATEGORY::ALL_CATEGORIES),
    month(PUBLISHER_MONTH::ANY),
    year(-1) {}
PublisherInfoFilter::PublisherInfoFilter(const PublisherInfoFilter& filter) :
    id(filter.id),
    category(filter.category),
    month(filter.month),
    year(filter.year) {}
PublisherInfoFilter::~PublisherInfoFilter() {}

PublisherInfo::PublisherInfo(const id_type& publisher_id,
                             PUBLISHER_MONTH _month,
                             int _year) :
    id(publisher_id),
    duration(0u),
    score(.0),
    visits(0u),
    pinned(false),
    percent(0u),
    weight(.0),
    excluded(PUBLISHER_EXCLUDE::DEFAULT),
    category(PUBLISHER_CATEGORY::AUTO_CONTRIBUTE),
    month(_month),
    year(_year),
    verified(false) {}

PublisherInfo::PublisherInfo(const PublisherInfo& info) :
    id(info.id),
    duration(info.duration),
    score(info.score),
    visits(info.visits),
    pinned(info.pinned),
    percent(info.percent),
    weight(info.weight),
    excluded(info.excluded),
    category(info.category),
    month(info.month),
    year(info.year),
    favIconURL(info.favIconURL),
    verified(info.verified),
    contributions(info.contributions) {}

PublisherInfo::~PublisherInfo() {}

bool PublisherInfo::is_valid() const {
  return !id.empty() && year > 0 && month != PUBLISHER_MONTH::ANY;
}

const PublisherInfo invalid("", PUBLISHER_MONTH::ANY, -1);


TwitchEventInfo::TwitchEventInfo() {}

TwitchEventInfo::TwitchEventInfo(const TwitchEventInfo& info):
  event_(info.event_),
  time_(info.time_),
  status_(info.status_) {}

TwitchEventInfo::~TwitchEventInfo() {}


MediaPublisherInfo::MediaPublisherInfo(const std::string& publisher_id):
  publisher_id_(publisher_id) {}

MediaPublisherInfo::MediaPublisherInfo(const MediaPublisherInfo& info):
  publisher_id_(info.publisher_id_),
  publisherName_(info.publisherName_),
  publisherURL_(info.publisherURL_),
  favIconURL_(info.favIconURL_),
  channelName_(info.channelName_),
  publisher_(info.publisher_),
  twitchEventInfo_(info.twitchEventInfo_) {}

MediaPublisherInfo::~MediaPublisherInfo() {}

const MediaPublisherInfo MediaPublisherInfo::FromJSON(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  if (d.HasParseError() ||
      !d["id"].IsString() ||
      !d["publisherName"].IsString() ||
      !d["publisherURL"].IsString() ||
      !d["favIconURL"].IsString() ||
      !d["channelName"].IsString() ||
      !d["publisher"].IsString() ||
      !d["twitch_event"].IsString() ||
      !d["twitch_time"].IsString() ||
      !d["twitch_status"].IsString()) {
    return MediaPublisherInfo("");
  }

  MediaPublisherInfo info(d["id"].GetString());
  info.publisherName_ = d["publisherName"].GetString();
  info.publisherURL_ = d["publisherURL"].GetString();
  info.favIconURL_ = d["favIconURL"].GetString();
  info.channelName_ = d["channelName"].GetString();
  info.publisher_ = d["publisher"].GetString();
  info.twitchEventInfo_.event_ = d["twitch_event"].GetString();
  info.twitchEventInfo_.time_ = d["twitch_time"].GetString();
  info.twitchEventInfo_.status_ = d["twitch_status"].GetString();

  return info;
}

const std::string MediaPublisherInfo::ToJSON() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("id");
  writer.String(publisher_id_.c_str());

  writer.String("publisherName");
  writer.String(publisherName_.c_str());

  writer.String("publisherURL");
  writer.String(publisherURL_.c_str());

  writer.String("favIconURL");
  writer.String(favIconURL_.c_str());

  writer.String("channelName");
  writer.String(channelName_.c_str());

  writer.String("publisher");
  writer.String(publisher_.c_str());

  writer.String("twitch_event");
  writer.String(twitchEventInfo_.event_.c_str());

  writer.String("twitch_time");
  writer.String(twitchEventInfo_.time_.c_str());

  writer.String("twitch_status");
  writer.String(twitchEventInfo_.status_.c_str());

  writer.EndObject();

  return buffer.GetString();
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

Grant::Grant () {}
Grant::~Grant () {}
Grant::Grant (const ledger::Grant &properties) {
  promotionId = properties.promotionId;
  expiryTime = properties.expiryTime;
  probi = properties.probi;
  altcurrency = properties.altcurrency;
}

BalanceReportInfo::BalanceReportInfo():
  opening_balance_(.0),
  closing_balance_(.0),
  grants_(.0),
  earning_from_ads_(.0),
  auto_contribute_(.0),
  recurring_donation_(.0),
  one_time_donation_(.0) {}

bool Ledger::IsMediaLink(const std::string& url, const std::string& first_party_url, const std::string& referrer) {
  return braveledger_bat_get_media::BatGetMedia::GetLinkType(url, first_party_url, referrer) == TWITCH_MEDIA_TYPE;
}
}

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

bool is_production = true;
int reconcile_time = 0; // minutes
bool short_retries = false;

VisitData::VisitData():
    tab_id(-1) {}

VisitData::VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id,
            PUBLISHER_MONTH _local_month,
            int _local_year,
            const std::string& _name,
            const std::string& _url,
            const std::string& _provider,
            const std::string& _favicon_url) :
    tld(_tld),
    domain(_domain),
    path(_path),
    tab_id(_tab_id),
    local_month(_local_month),
    local_year(_local_year),
    name(_name),
    url(_url),
    provider(_provider),
    favicon_url(_favicon_url) {}

VisitData::VisitData(const VisitData& data) :
    tld(data.tld),
    domain(data.domain),
    path(data.path),
    tab_id(data.tab_id),
    local_month(data.local_month),
    local_year(data.local_year),
    name(data.name),
    url(data.url),
    provider(data.provider),
    favicon_url(data.favicon_url) {}

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
    year(-1),
    excluded(PUBLISHER_EXCLUDE_FILTER::FILTER_DEFAULT),
    min_duration(0),
    reconcile_stamp(0) {}
PublisherInfoFilter::PublisherInfoFilter(const PublisherInfoFilter& filter) :
    id(filter.id),
    category(filter.category),
    month(filter.month),
    year(filter.year),
    excluded(filter.excluded),
    order_by(filter.order_by),
    min_duration(filter.min_duration),
    reconcile_stamp(filter.reconcile_stamp) {}
PublisherInfoFilter::~PublisherInfoFilter() {}

PublisherBanner::PublisherBanner() {}

PublisherBanner::PublisherBanner(const PublisherBanner& info) :
    publisher_key(info.publisher_key),
    title(info.title),
    name(info.name),
    description(info.description),
    background(info.background),
    logo(info.logo),
    amounts(info.amounts),
    social(info.social) {}

PublisherBanner::~PublisherBanner() {}

PublisherInfo::PublisherInfo() :
    duration(0u),
    score(.0),
    visits(0u),
    percent(0u),
    weight(.0),
    excluded(PUBLISHER_EXCLUDE::DEFAULT),
    category(PUBLISHER_CATEGORY::AUTO_CONTRIBUTE),
    month(PUBLISHER_MONTH::ANY),
    year(-1),
    reconcile_stamp(0),
    verified(false),
    name(""),
    url(""),
    provider(""),
    favicon_url("") {}

PublisherInfo::PublisherInfo(const std::string& publisher_id,
                             PUBLISHER_MONTH _month,
                             int _year) :
    id(publisher_id),
    duration(0u),
    score(.0),
    visits(0u),
    percent(0u),
    weight(.0),
    excluded(PUBLISHER_EXCLUDE::DEFAULT),
    category(PUBLISHER_CATEGORY::AUTO_CONTRIBUTE),
    month(_month),
    year(_year),
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
    month(info.month),
    year(info.year),
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
  return !id.empty() && year > 0 && month != PUBLISHER_MONTH::ANY;
}

const PublisherInfo invalid("", PUBLISHER_MONTH::ANY, -1);


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

bool Ledger::IsMediaLink(const std::string& url, const std::string& first_party_url, const std::string& referrer) {
  return braveledger_bat_get_media::BatGetMedia::GetLinkType(url, first_party_url, referrer) == TWITCH_MEDIA_TYPE;
}
}

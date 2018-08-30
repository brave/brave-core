/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger.h"

#include "ledger_impl.h"

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
    local_year(data.local_year) {}

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
    excluded(false),
    category(PUBLISHER_CATEGORY::AUTO_CONTRIBUTE),
    month(_month),
    year(_year) {}

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
    contributions(info.contributions) {}

PublisherInfo::~PublisherInfo() {}

bool PublisherInfo::is_valid() const {
  return !id.empty() && year > 0 && month != PUBLISHER_MONTH::ANY;
}

const PublisherInfo invalid("", PUBLISHER_MONTH::ANY, -1);

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
}

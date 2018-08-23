/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PUBLISHER_INFO_HANDLER_
#define BAT_LEDGER_PUBLISHER_INFO_HANDLER_

#include <string>
#include <vector>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT enum PUBLISHER_CATEGORY {
  ALL_CATEGORIES = 1 << 0,
  AUTO_CONTRIBUTE = 1 << 1,
  TIPPING = 1 << 2,
  DIRECT_DONATION = 1 << 3,
  RECURRING_DONATION = 1 << 4
};

LEDGER_EXPORT enum PUBLISHER_MONTH {
  JANUARY = 1,
  FEBRUARY = 2,
  MARCH = 3,
  APRIL = 4,
  MAY = 5,
  JUNE = 6,
  JULY = 7,
  AUGUST = 8,
  SEPTEMBER = 9,
  OCTOBER = 10,
  NOVEMBER = 11,
  DECEMBER = 12
};

LEDGER_EXPORT struct PublisherInfoFilter {
  PublisherInfoFilter(int category_, 
    PUBLISHER_MONTH month_, const std::string& year_);

  int category;
  PUBLISHER_MONTH month;
  std::string year;
};

LEDGER_EXPORT struct ContributionInfo {
  ContributionInfo() {}
  ContributionInfo(const double &value_, const uint64_t& date_):
    value(value_),
    date(date_) {}

  std::string publisher;  // Filled only for recurrent donations
  double value;
  uint64_t date;
};

LEDGER_EXPORT struct PublisherInfo {
  typedef std::string id_type;
  PublisherInfo(const id_type& publisher_id);
  PublisherInfo(const PublisherInfo& info);
  ~PublisherInfo();

  static const PublisherInfo FromJSON(const std::string& json);
  const std::string ToJSON() const;
  //bool Matches(PublisherInfoFilter filter) const;
  bool is_valid() const;

  const id_type id;
  uint64_t duration;
  double score;
  uint32_t visits;
  bool pinned;
  uint32_t percent;
  double weight;
  bool excluded;
  std::string key;
  std::vector<ContributionInfo> contributions;
  PUBLISHER_CATEGORY category;
  PUBLISHER_MONTH month;
  std::string year;
};

using PublisherInfoList = std::vector<const PublisherInfo>;

}  // namespace ledger

#endif  // BAT_LEDGER_PUBLISHER_INFO_HANDLER_

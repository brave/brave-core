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
  AUTO_CONTRIBUTE = 1 << 1,
  TIPPING = 1 << 2,
  DIRECT_DONATION = 1 << 3,
  RECURRING_DONATION = 1 << 4,
  ALL_CATEGORIES = (1 << 5) - 1,
};

LEDGER_EXPORT enum PUBLISHER_MONTH {
  ANY = -1,
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

LEDGER_EXPORT enum PUBLISHER_EXCLUDE {
  DEFAULT = 0, // this tell us that user did not manually changed exclude state
  EXCLUDED = 1, // user manually changed it to exclude
  INCLUDED = 2 // user manually changed it to include and is overriding server flags
};

LEDGER_EXPORT struct PublisherInfoFilter {
  PublisherInfoFilter();
  PublisherInfoFilter(const PublisherInfoFilter& filter);
  ~PublisherInfoFilter();

  std::string id;
  int category;
  PUBLISHER_MONTH month;
  int year;
  PUBLISHER_EXCLUDE excluded;

  std::vector<std::pair<std::string, bool>> order_by;
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
  PublisherInfo();
  PublisherInfo(const std::string& publisher_id, PUBLISHER_MONTH month, int year);
  PublisherInfo(const PublisherInfo& info);
  ~PublisherInfo();

  bool is_valid() const;

  std::string id;
  uint64_t duration;
  double score;
  uint32_t visits;
  uint32_t percent;
  double weight;
  PUBLISHER_EXCLUDE excluded;
  PUBLISHER_CATEGORY category;
  PUBLISHER_MONTH month;
  int year;
  bool verified;
  std::string name;
  std::string url;
  std::string provider;
  std::string favicon_url;

  std::vector<ContributionInfo> contributions;
};

using PublisherInfoList = std::vector<PublisherInfo>;

}  // namespace ledger

#endif  // BAT_LEDGER_PUBLISHER_INFO_HANDLER_

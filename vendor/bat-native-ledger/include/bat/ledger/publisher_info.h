/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PUBLISHER_INFO_HANDLER_
#define BAT_LEDGER_PUBLISHER_INFO_HANDLER_

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "bat/ledger/export.h"

namespace ledger {

const char _clear_favicon[] = "clear";

LEDGER_EXPORT enum REWARDS_CATEGORY {
  AUTO_CONTRIBUTE = 1 << 1,  // 2
  ONE_TIME_TIP = 1 << 3,  // 8
  RECURRING_TIP = 1 << 4,  // 21
  ALL_CATEGORIES = (1 << 5) - 1,
};

LEDGER_EXPORT enum ACTIVITY_MONTH {
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
  ALL = -1,
  DEFAULT = 0,  // this tell us that user did not manually changed exclude state
  EXCLUDED = 1,  // user manually changed it to exclude
  INCLUDED = 2  // user manually changed it to include and is overriding server
};

LEDGER_EXPORT enum EXCLUDE_FILTER {
  FILTER_ALL = -1,
  FILTER_DEFAULT = 0,
  FILTER_EXCLUDED = 1,
  FILTER_INCLUDED = 2,
  FILTER_ALL_EXCEPT_EXCLUDED = 3
};

LEDGER_EXPORT struct ActivityInfoFilter {
  ActivityInfoFilter();
  ActivityInfoFilter(const ActivityInfoFilter& filter);
  ~ActivityInfoFilter();

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string id;
  EXCLUDE_FILTER excluded;
  uint32_t percent;
  std::vector<std::pair<std::string, bool>> order_by;
  uint64_t min_duration;
  uint64_t reconcile_stamp;
  bool non_verified;
  uint32_t min_visits;
};

LEDGER_EXPORT struct ContributionInfo {
  ContributionInfo() {}
  ContributionInfo(const double &value_, const uint64_t& date_):
    value(value_),
    date(date_) {}

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string publisher;  // Filled only for recurrent donations
  double value;
  uint64_t date;
};

LEDGER_EXPORT struct PublisherBanner {
  PublisherBanner();
  PublisherBanner(const PublisherBanner& info);
  ~PublisherBanner();

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string publisher_key;
  std::string title;
  std::string name;
  std::string description;
  std::string background;
  std::string logo;
  std::vector<int> amounts;
  std::string provider;
  std::map<std::string, std::string> social;
  bool verified;
};

LEDGER_EXPORT struct PublisherInfo {
  PublisherInfo();
  PublisherInfo(const std::string& publisher_id);
  PublisherInfo(const PublisherInfo& info);
  ~PublisherInfo();

  bool operator<(const PublisherInfo& rhs) const;
  bool is_valid() const;

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string id;
  uint64_t duration;
  double score;
  uint32_t visits;
  uint32_t percent;
  double weight;
  PUBLISHER_EXCLUDE excluded;
  REWARDS_CATEGORY category;
  uint64_t reconcile_stamp;
  bool verified;
  std::string name;
  std::string url;
  std::string provider;
  std::string favicon_url;

  std::vector<ContributionInfo> contributions;
};

LEDGER_EXPORT struct PublisherInfoListStruct {
  PublisherInfoListStruct();
  ~PublisherInfoListStruct();
  PublisherInfoListStruct(const PublisherInfoListStruct& data);

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::vector<PublisherInfo> list;
};

using PublisherInfoList = std::vector<PublisherInfo>;

}  // namespace ledger

#endif  // BAT_LEDGER_PUBLISHER_INFO_HANDLER_

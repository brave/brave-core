/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_PUBLISHERS_H_
#define BRAVELEDGER_BAT_PUBLISHERS_H_

#include "bat_helper.h"
#include "base/callback.h"
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <memory>

#include "bat_helper.h"


#if defined CHROMIUM_BUILD
#include "base/callback.h"
#else
#include <functional>
#endif

#include "leveldb/db.h"

namespace braveledger_bat_publishers {

class BatPublishers {
public:

#if defined CHROMIUM_BUILD
  typedef base::Callback<void(const std::string&, const uint64_t&)> SaveVisitCallback;
#else
  typedef std::function<void(const std::string&, const uint64_t&)> SaveVisitCallback;
#endif  

  BatPublishers();

  ~BatPublishers();

  void initSynopsis();
  void saveVisit(const std::string& publisher, const uint64_t& duration,
    BatPublishers::SaveVisitCallback callback, bool ignoreMinTime);
  void setPublisherTimestampVerified(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified);
  void setPublisherFavIcon(const std::string& publisher, const std::string& favicon_url);
  void setPublisherInclude(const std::string& publisher, const bool& include);
  void setPublisherDeleted(const std::string& publisher, const bool& deleted);
  void setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage);
  void setPublisherMinVisitTime(const uint64_t& duration); // In milliseconds
  void setPublisherMinVisits(const unsigned int& visits);
  void setPublisherAllowNonVerified(const bool& allow);
  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> getPublishersData();
  std::vector<braveledger_bat_helper::WINNERS_ST> winners(const unsigned int& ballots);
  bool isEligableForContribution(const braveledger_bat_helper::PUBLISHER_DATA_ST& publisherData);

private:
  double concaveScore(const uint64_t& duration);
  void openPublishersDB();
  void loadPublishers();
  void saveVisitInternal(const std::string& publisher, const uint64_t& duration,
    BatPublishers::SaveVisitCallback callback);
  void setPublisherFavIconInternal(const std::string& publisher, const std::string& favicon_url);
  void setPublisherTimestampVerifiedInternal(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified);
  void setPublisherDeletedInternal(const std::string& publisher, const bool& deleted);
  void setPublisherIncludeInternal(const std::string& publisher, const bool& include);
  void setPublisherPinPercentageInternal(const std::string& publisher, const bool& pinPercentage);
  void loadStateCallback(bool result, const braveledger_bat_helper::PUBLISHER_STATE_ST& state);
  void calcScoreConsts();
  void synopsisNormalizer();
  void synopsisNormalizerInternal();
  bool isPublisherVisible(const braveledger_bat_helper::PUBLISHER_ST& publisher_st);
  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> topN();

  std::map<std::string, braveledger_bat_helper::PUBLISHER_ST> publishers_;

  std::mutex publishers_map_mutex_;

  std::unique_ptr<leveldb::DB> level_db_;

  braveledger_bat_helper::PUBLISHER_STATE_ST state_;

  unsigned int a_;
  unsigned int a2_;
  unsigned int a4_;
  unsigned int b_;
  unsigned int b2_;
};

} //namespace braveledger_bat_publishers

#endif  // BRAVELEDGER_BAT_PUBLISHERS_H_

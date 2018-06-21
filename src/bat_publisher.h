/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_PUBLISHER_H_
#define BAT_PUBLISHER_H_

#include "bat_helper.h"
#include "base/callback.h"
#include <string>
#include <map>
#include <mutex>
#include <vector>

namespace leveldb {
class DB;
}

namespace bat_publisher {

class BatPublisher {
public:
  typedef base::Callback<void(const std::string&, const uint64_t&)> SaveVisitCallback;

  BatPublisher();
  ~BatPublisher();

  void initSynopsis();
  void saveVisit(const std::string& publisher, const uint64_t& duration,
    BatPublisher::SaveVisitCallback callback);
  void setPublisherTimestampVerified(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified);
  void setPublisherFavIcon(const std::string& publisher, const std::string& favicon_url);
  void setPublisherInclude(const std::string& publisher, const bool& include);
  void setPublisherDeleted(const std::string& publisher, const bool& deleted);
  void setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage);
  void setPublisherMinVisitTime(const uint64_t& duration); // In milliseconds
  void setPublisherMinVisits(const unsigned int& visits);
  void setPublisherAllowNonVerified(const bool& allow);
  std::vector<PUBLISHER_DATA_ST> getPublishersData();
  std::vector<WINNERS_ST> winners(const unsigned int& ballots);
  bool isEligableForContribution(const PUBLISHER_DATA_ST& publisherData);

private:
  double concaveScore(const uint64_t& duration);
  void openPublishersDB();
  void loadPublishers();
  void saveVisitInternal(const std::string& publisher, const uint64_t& duration,
    BatPublisher::SaveVisitCallback callback);
  void setPublisherFavIconInternal(const std::string& publisher, const std::string& favicon_url);
  void setPublisherTimestampVerifiedInternal(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified);
  void setPublisherDeletedInternal(const std::string& publisher, const bool& deleted);
  void setPublisherIncludeInternal(const std::string& publisher, const bool& include);
  void setPublisherPinPercentageInternal(const std::string& publisher, const bool& pinPercentage);
  void loadStateCallback(bool result, const PUBLISHER_STATE_ST& state);
  void calcScoreConsts();
  void synopsisNormalizer();
  void synopsisNormalizerInternal();
  bool isPublisherVisible(const PUBLISHER_ST& publisher_st);
  std::vector<PUBLISHER_DATA_ST> topN();

  std::map<std::string, PUBLISHER_ST> publishers_;
  std::mutex publishers_map_mutex_;
  leveldb::DB* level_db_;

  PUBLISHER_STATE_ST state_;
  unsigned int a_;
  unsigned int a2_;
  unsigned int a4_;
  unsigned int b_;
  unsigned int b2_;
};

}

#endif  // BAT_PUBLISHER_H_

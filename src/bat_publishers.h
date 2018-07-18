/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_PUBLISHERS_H_
#define BRAVELEDGER_BAT_PUBLISHERS_H_

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "bat/ledger/ledger_callback_handler.h"
#include "bat_helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace leveldb {
class DB;
}

namespace braveledger_bat_helper {
struct PUBLISHER_STATE_ST;
}

namespace braveledger_bat_publishers {

class BatPublishers : public ledger::LedgerCallbackHandler {
 public:

  BatPublishers(bat_ledger::LedgerImpl* ledger);

  ~BatPublishers() override;

  void initSynopsis();

  void saveVisit(std::string publisher, uint64_t duration, braveledger_bat_helper::SaveVisitCallback callback, bool ignoreMinTime);

  void setPublisherTimestampVerified(std::string publisher, uint64_t verifiedTimestamp, bool verified);

  void setPublisherFavIcon(std::string publisher, std::string favicon_url);

  void setPublisherInclude(std::string publisher, bool include);

  void setPublisherDeleted(std::string publisher, bool deleted);

  void setPublisherPinPercentage(std::string publisher, bool pinPercentage);

  void setPublisherMinVisitTime(const uint64_t& duration); // In milliseconds

  void setPublisherMinVisits(const unsigned int& visits);

  void setPublisherAllowNonVerified(const bool& allow);

  uint64_t getPublisherMinVisitTime() const; // In milliseconds
  unsigned int getPublisherMinVisits() const;
  bool getPublisherAllowNonVerified() const;

  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> getPublishersData();

  std::vector<braveledger_bat_helper::WINNERS_ST> winners(const unsigned int& ballots);

  bool isEligableForContribution(const braveledger_bat_helper::PUBLISHER_DATA_ST& publisherData);

  void loadState(bool success, const std::string& data);

 private:
  // LedgerCallbackHandler impl
  void OnLedgerStateLoaded(ledger::Result result,
                           const std::string& data) override;
  void OnPublisherStateLoaded(ledger::Result result,
                              const std::string& data) override;

  double concaveScore(const uint64_t& duration);

  void saveState();

  bool Init();
  bool EnsureInitialized();

  void loadPublishers();

  void saveVisitInternal(const std::string& publisher, uint64_t duration,
    braveledger_bat_helper::SaveVisitCallback callback);

  void setPublisherFavIconInternal(const std::string& publisher, const std::string& favicon_url);

  void setPublisherTimestampVerifiedInternal(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified);

  void setPublisherDeletedInternal(const std::string& publisher, const bool& deleted);

  void setPublisherIncludeInternal(const std::string& publisher, const bool& include);

  void setPublisherPinPercentageInternal(const std::string& publisher, const bool& pinPercentage);

  void calcScoreConsts();

  void synopsisNormalizer();

  void synopsisNormalizerInternal();

  bool isPublisherVisible(const braveledger_bat_helper::PUBLISHER_ST& publisher_st);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> topN();

  std::map<std::string, braveledger_bat_helper::PUBLISHER_ST> publishers_;

  std::mutex publishers_map_mutex_;

  std::unique_ptr<leveldb::DB> level_db_;

  std::unique_ptr<braveledger_bat_helper::PUBLISHER_STATE_ST> state_;

  unsigned int a_;

  unsigned int a2_;

  unsigned int a4_;

  unsigned int b_;

  unsigned int b2_;
};

}  // namespace braveledger_bat_publishers

#endif  // BRAVELEDGER_BAT_PUBLISHERS_H_

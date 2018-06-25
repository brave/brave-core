/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEDGER_H_
#define BRAVELEDGER_LEDGER_H_


#include <string>
#include <memory>
#include <map>

#include "bat_helper.h"
#include "bat_client.h"
#include "bat_publishers.h"
#include "bat_get_media.h"



namespace braveledger_ledger {

class Ledger {
public:
  Ledger();

  ~Ledger();

  // Not copyable, not assignable
  Ledger(const Ledger&) = delete;

  Ledger& operator=(const Ledger&) = delete;  

  void createWallet();

  void initSynopsis();

  void saveVisit(const std::string& publisher, const uint64_t& duration, bool ignoreMinTime);

  void saveVisitCallback(const std::string& publisher, const uint64_t& verifiedTimestamp);

  void favIconUpdated(const std::string& publisher, const std::string& favicon_url);

  void setPublisherInclude(const std::string& publisher, const bool& include);

  void setPublisherDeleted(const std::string& publisher, const bool& deleted);

  void setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage);

  void setPublisherMinVisitTime(const uint64_t& duration); // In milliseconds

  void setPublisherMinVisits(const unsigned int& visits);

  void setPublisherAllowNonVerified(const bool& allow);

  void setContributionAmount(const double& amount);

  void OnMediaRequest(const std::string& url, const std::string& urlQuery, const std::string& type, bool privateTab);

  std::string getBATAddress();

  std::string getBTCAddress();

  std::string getETHAddress();

  std::string getLTCAddress();

  void getBalance();

  void run(const uint64_t& delayTime);


private:

  bool isBatClientExist();

  bool isBatPublisherExist();

  void publisherInfoCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);

  void walletPropertiesCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);

  void reconcileCallback(const std::string& viewingId);

  void OnMediaRequestCallback(const uint64_t& duration, const braveledger_bat_helper::MEDIA_PUBLISHER_INFO& mediaPublisherInfo);

  void processMedia(const std::map<std::string, std::string>& parts, const std::string& type);

  std::unique_ptr<braveledger_bat_client::BatClient> bat_client_;

  std::unique_ptr<braveledger_bat_publishers::BatPublishers> bat_publishers_;

  braveledger_bat_get_media::BatGetMedia* bat_get_media_;
};

} //namespace braveledger_ledger

#endif  // BRAVELEDGER_LEDGER_H_

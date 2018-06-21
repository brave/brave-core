/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/task_scheduler/post_task.h"
#include "ledger.h"
#include "bat_client.h"
#include "bat_get_media.h"
#include "bat_publisher.h"
#include "base/bind.h"
#include "base/guid.h"
#include "static_values.h"
//#include "chrome/browser/io_thread.h"

#include "logging.h"


using namespace bat_client;
using namespace bat_publisher;
using namespace bat_get_media;

namespace ledger {

  Ledger::Ledger():
      bat_client_(nullptr),
      bat_publisher_(nullptr),
      bat_get_media_(nullptr) {
    bat_get_media_ = new BatGetMedia();
  }

  Ledger::~Ledger() {
    if (bat_client_) {
      delete bat_client_;
    }
    if (bat_publisher_) {
      delete bat_publisher_;
    }
    if (bat_get_media_) {
      delete bat_get_media_;
    }
  }

  void Ledger::createWallet() {
    initSynopsis();
    if (!bat_client_) {
      bat_client_ = new BatClient();
    }
    LOG(ERROR) << "!!!here2";
    bat_client_->loadStateOrRegisterPersona();
  }

  void Ledger::initSynopsis() {
    if (!bat_publisher_) {
      bat_publisher_ = new BatPublisher();
    }
    bat_publisher_->initSynopsis();
  }

  bool Ledger::isBatClientExist() {
    if (!bat_client_) {
      LOG(ERROR) << "ledger bat_client is not exist";

      return false;
    }

    return true;
  }

  bool Ledger::isBatPublisherExist() {
    if (!bat_publisher_) {
      LOG(ERROR) << "ledger bat_publisher is not exist";

      return false;
    }

    return true;
  }

  void Ledger::getBalance() {
    if (!isBatClientExist()) {
      assert(false);

      return;
    }
    FETCH_CALLBACK_EXTRA_DATA_ST extraData;
    bat_client_->getWalletProperties(base::Bind(&Ledger::walletPropertiesCallback,
      base::Unretained(this)), extraData);
  }

  void Ledger::walletPropertiesCallback(bool result, const std::string& response, const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
    if (!result) {
      // TODO errors handling
      return;
    }
    WALLET_PROPERTIES_ST walletProperties;
    BatHelper::getJSONWalletProperties(response, walletProperties);
    // TODO send the balance to the UI via observer or callback
  }

  void Ledger::saveVisit(const std::string& publisher, const uint64_t& duration, bool ignoreMinTime) {
    // TODO debug
    //bat_client_->recoverWallet(bat_client_->getWalletPassphrase());
    //
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->saveVisit(publisher, duration, base::Bind(&Ledger::saveVisitCallback,
      base::Unretained(this)), ignoreMinTime);
  }

  void Ledger::saveVisitCallback(const std::string& publisher, const uint64_t& verifiedTimestamp) {
    if (!isBatClientExist()) {
      assert(false);

      return;
    }
    uint64_t publisherTimestamp = bat_client_->getPublisherTimestamp();
    if (publisherTimestamp <= verifiedTimestamp) {
      //to do debug
      //LOG(ERROR) << "!!!reconcile";
      //run(0);
      //
      return;
    }
    FETCH_CALLBACK_EXTRA_DATA_ST extraData;
    extraData.value1 = publisherTimestamp;
    extraData.string1 = publisher;
    // Update publisher verified or not flag
    //LOG(ERROR) << "!!!getting publisher info";
    bat_client_->publisherInfo(publisher, base::Bind(&Ledger::publisherInfoCallback,
      base::Unretained(this)), extraData);
  }

  void Ledger::publisherInfoCallback(bool result, const std::string& response,
      const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
    //LOG(ERROR) << "!!!got publisher info";
    if (!result) {
      // TODO errors handling
      return;
    }
    bool verified = false;
    BatHelper::getJSONPublisherVerified(response, verified);
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherTimestampVerified(extraData.string1, extraData.value1, verified);
    //to do debug
    //LOG(ERROR) << "!!!reconcile";
    //run(0);
    //
  }

  void Ledger::favIconUpdated(const std::string& publisher, const std::string& favicon_url) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherFavIcon(publisher, favicon_url);
  }

  void Ledger::setPublisherInclude(const std::string& publisher, const bool& include) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherInclude(publisher, include);
  }

  void Ledger::setPublisherDeleted(const std::string& publisher, const bool& deleted) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherDeleted(publisher, deleted);
  }

  void Ledger::setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherPinPercentage(publisher, pinPercentage);
  }

  void Ledger::setPublisherMinVisitTime(const uint64_t& duration) { // In milliseconds
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherMinVisitTime(duration);
  }

  void Ledger::setPublisherMinVisits(const unsigned int& visits) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherMinVisits(visits);
  }

  void Ledger::setPublisherAllowNonVerified(const bool& allow) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publisher_->setPublisherAllowNonVerified(allow);
  }

  void Ledger::setContributionAmount(const double& amount) {
    if (!isBatClientExist()) {
      assert(false);

      return;
    }
    bat_client_->setContributionAmount(amount);
  }

  std::string Ledger::getBATAddress() {
    if (!isBatClientExist()) {
      assert(false);

      return "";
    }
    return bat_client_->getBATAddress();
  }

  std::string Ledger::getBTCAddress() {
    if (!isBatClientExist()) {
      assert(false);

      return "";
    }
    return bat_client_->getBTCAddress();
  }

  std::string Ledger::getETHAddress() {
    if (!isBatClientExist()) {
      assert(false);

      return "";
    }
    return bat_client_->getETHAddress();
  }

  std::string Ledger::getLTCAddress() {
    if (!isBatClientExist()) {
      assert(false);

      return "";
    }
    return bat_client_->getLTCAddress();
  }

  void Ledger::run(const uint64_t& delayTime) {
    // That function should be triggeres from the main process periodically to make payments
    if (!isBatClientExist()) {
      assert(false);

      return;
    }
    if (bat_client_->isReadyForReconcile()) {
      bat_client_->reconcile(base::GenerateGUID(), base::Bind(&Ledger::reconcileCallback,
        base::Unretained(this)));
    }
  }

  void Ledger::reconcileCallback(const std::string& viewingId) {
    if (!isBatClientExist() || !isBatPublisherExist()) {
      assert(false);

      return;
    }
    LOG(ERROR) << "!!!in reconcile callback";
    unsigned int ballotsCount = bat_client_->ballots("");
    LOG(ERROR) << "!!!ballotsCount == " << ballotsCount;
    std::vector<WINNERS_ST> winners = bat_publisher_->winners(ballotsCount);
    std::vector<std::string> publishers;
    for (size_t i = 0; i < winners.size(); i++) {
      if (!bat_publisher_->isEligableForContribution(winners[i].publisher_data_)) {
        continue;
      }
      publishers.push_back(winners[i].publisher_data_.publisherKey_);
    }
    bat_client_->votePublishers(publishers, ""/*, viewingId*/);
    // TODO call prepareBallots by timeouts like in js library
    bat_client_->prepareBallots();
  }

  void Ledger::OnMediaRequest(const std::string& url, const std::string& urlQuery, const std::string& type, bool privateTab) {
    // Don't track private tabs
    if (privateTab) {
      return;
    }
    //LOG(ERROR) << "!!!media url == " << url;
    //LOG(ERROR) << "!!!media urlQuery == " << urlQuery;
    //LOG(ERROR) << "!!!media url type == " << type;
    std::map<std::string, std::string> parts;
    std::vector<std::map<std::string, std::string>> twitchParts;
    if (YOUTUBE_MEDIA_TYPE == type) {
      BatHelper::getUrlQueryParts(urlQuery, parts);
      processMedia(parts, type);
    } else if (TWITCH_MEDIA_TYPE == type) {
      BatHelper::getTwitchParts(urlQuery, twitchParts);
      for (size_t i = 0; i < twitchParts.size(); i++) {
        processMedia(twitchParts[i], type);
      }
    }
  }

  void Ledger::processMedia(const std::map<std::string, std::string>& parts, const std::string& type) {
    std::string mediaId = BatHelper::getMediaId(parts, type);
    //LOG(ERROR) << "!!!mediaId == " << mediaId;
    if (mediaId.empty()) {
      return;
    }
    std::string mediaKey = BatHelper::getMediaKey(mediaId, type);
    //LOG(ERROR) << "!!!mediaKey == " << mediaKey;
    uint64_t duration = 0;
    TWITCH_EVENT_INFO twitchEventInfo;
    if (YOUTUBE_MEDIA_TYPE == type) {
      duration = BatHelper::getMediaDuration(parts, mediaKey, type);
      //LOG(ERROR) << "!!!duration == " << duration;
    } else if (TWITCH_MEDIA_TYPE == type) {
      std::map<std::string, std::string>::const_iterator iter = parts.find("event");
      if (iter != parts.end()) {
        twitchEventInfo.event_ = iter->second;
      }
      iter = parts.find("time");
      if (iter != parts.end()) {
        twitchEventInfo.time_ = iter->second;
      }
    }
    if (!bat_get_media_) {
      return;
    }
    scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    task_runner->PostTask(FROM_HERE, base::Bind(&BatGetMedia::getPublisherFromMediaProps, base::Unretained(bat_get_media_), 
      mediaId, mediaKey, type, duration, twitchEventInfo, base::Bind(&Ledger::OnMediaRequestCallback,
      base::Unretained(this))));
  }

  void Ledger::OnMediaRequestCallback(const uint64_t& duration, const MEDIA_PUBLISHER_INFO& mediaPublisherInfo) {
    saveVisit(mediaPublisherInfo.publisher_, duration, true);
  }

}

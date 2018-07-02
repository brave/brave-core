/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "bat_client.h"
#include "bat_get_media.h"
#include "bat_helper.h"
#include "bat_publishers.h"
#include "ledger.h"
#include "static_values.h"

#include "rapidjson_bat_helper.h"

using namespace braveledger_bat_client;
using namespace braveledger_bat_publishers;
using namespace braveledger_bat_get_media;

namespace braveledger_ledger {

  Ledger::Ledger()
  {
    bat_get_media_ = new BatGetMedia();
  }

  Ledger::~Ledger() {
    if (bat_get_media_) {
      delete bat_get_media_;
    }
  }

  void Ledger::createWallet() {
    initSynopsis();
    if (!bat_client_) {
      bat_client_.reset (new BatClient());
    }
    LOG(ERROR) << "here 2";
    bat_client_->loadStateOrRegisterPersona();
  }

  void Ledger::initSynopsis() {
    if (!bat_publishers_) {
      bat_publishers_.reset(new BatPublishers());
    }
    bat_publishers_->initSynopsis();
  }

  bool Ledger::isBatClientExist() {
    if (!bat_client_) {
      LOG(ERROR) << "ledger bat_client is not exist";

      return false;
    }

    return true;
  }

  bool Ledger::isBatPublisherExist() {
    if (!bat_publishers_) {
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
    braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST extraData;
    auto runnable = braveledger_bat_helper::bat_mem_fun_binder3(*this, &Ledger::walletPropertiesCallback);
    bat_client_->getWalletProperties(runnable, extraData);
  }

  void Ledger::walletPropertiesCallback(bool result, const std::string& response, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
    if (!result) {
      // TODO errors handling
      return;
    }
    braveledger_bat_helper::WALLET_PROPERTIES_ST walletProperties;
    braveledger_bat_helper::loadFromJson(walletProperties, response);
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
    auto runnable = braveledger_bat_helper::bat_mem_fun_binder2(*this, &Ledger::saveVisitCallback);
    bat_publishers_->saveVisit(publisher, duration, runnable, ignoreMinTime);
  }

  void Ledger::saveVisitCallback(const std::string& publisher, const uint64_t& verifiedTimestamp) {
    if (!isBatClientExist()) {
      assert(false);

      return;
    }
    uint64_t publisherTimestamp = bat_client_->getPublisherTimestamp();
    if (publisherTimestamp <= verifiedTimestamp) {
      //to do debug
      LOG(ERROR) << "!!!reconcile";
      run(0);
      //
      return;
    }
    braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST extraData;
    extraData.value1 = publisherTimestamp;
    extraData.string1 = publisher;
    // Update publisher verified or not flag
    //LOG(ERROR) << "!!!getting publisher info";
    auto runnable = braveledger_bat_helper::bat_mem_fun_binder3(*this, &Ledger::publisherInfoCallback);
    bat_client_->publisherInfo(publisher, runnable, extraData);
  }

  void Ledger::publisherInfoCallback(bool result, const std::string& response,
    const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
    //LOG(ERROR) << "!!!got publisher info";
    if (!result) {
      // TODO errors handling
      return;
    }
    bool verified = false;
    braveledger_bat_helper::getJSONPublisherVerified(response, verified);
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherTimestampVerified(extraData.string1, extraData.value1, verified);
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
    bat_publishers_->setPublisherFavIcon(publisher, favicon_url);
  }

  void Ledger::setPublisherInclude(const std::string& publisher, const bool& include) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherInclude(publisher, include);
  }

  void Ledger::setPublisherDeleted(const std::string& publisher, const bool& deleted) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherDeleted(publisher, deleted);
  }

  void Ledger::setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherPinPercentage(publisher, pinPercentage);
  }

  void Ledger::setPublisherMinVisitTime(const uint64_t& duration) { // In milliseconds
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherMinVisitTime(duration);
  }

  void Ledger::setPublisherMinVisits(const unsigned int& visits) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherMinVisits(visits);
  }

  void Ledger::setPublisherAllowNonVerified(const bool& allow) {
    if (!isBatPublisherExist()) {
      assert(false);

      return;
    }
    bat_publishers_->setPublisherAllowNonVerified(allow);
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
      auto runnable = braveledger_bat_helper::bat_mem_fun_binder1(*this, &Ledger::reconcileCallback);
      bat_client_->reconcile(braveledger_bat_helper::GenerateGUID(), runnable);
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
    std::vector<braveledger_bat_helper::WINNERS_ST> winners = bat_publishers_->winners(ballotsCount);
    std::vector<std::string> publishers;
    for (size_t i = 0; i < winners.size(); i++) {
      if (!bat_publishers_->isEligableForContribution(winners[i].publisher_data_)) {
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
      braveledger_bat_helper::getUrlQueryParts(urlQuery, parts);
      processMedia(parts, type);
    } else if (TWITCH_MEDIA_TYPE == type) {
      braveledger_bat_helper::getTwitchParts(urlQuery, twitchParts);
      for (size_t i = 0; i < twitchParts.size(); i++) {
        processMedia(twitchParts[i], type);
      }
    }
  }

  void Ledger::processMedia(const std::map<std::string, std::string>& parts, const std::string& type) {
    std::string mediaId = braveledger_bat_helper::getMediaId(parts, type);
    //LOG(ERROR) << "!!!mediaId == " << mediaId;
    if (mediaId.empty()) {
      return;
    }
    std::string mediaKey = braveledger_bat_helper::getMediaKey(mediaId, type);
    //LOG(ERROR) << "!!!mediaKey == " << mediaKey;
    uint64_t duration = 0;
    braveledger_bat_helper::TWITCH_EVENT_INFO twitchEventInfo;
    if (YOUTUBE_MEDIA_TYPE == type) {
      duration = braveledger_bat_helper::getMediaDuration(parts, mediaKey, type);
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

    braveledger_bat_helper::GetMediaPublisherInfoCallback runnable1 = braveledger_bat_helper::bat_mem_fun_binder2(*this, &Ledger::OnMediaRequestCallback);

    auto runnable2 = braveledger_bat_helper::bat_mem_fun_binder(*bat_get_media_, &BatGetMedia::getPublisherFromMediaProps,
      std::cref(mediaId), std::cref(mediaKey), type, std::cref(duration), std::cref(twitchEventInfo), runnable1);

    braveledger_bat_helper::PostTask(runnable2);
  }

  void Ledger::OnMediaRequestCallback(const uint64_t& duration, const braveledger_bat_helper::MEDIA_PUBLISHER_INFO& mediaPublisherInfo) {
    saveVisit(mediaPublisherInfo.publisher_, duration, true);
  }

}  // namespace braveledger_ledger

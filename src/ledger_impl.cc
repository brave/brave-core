/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ledger_impl.h"

#include "bat_client.h"
#include "bat_get_media.h"
#include "bat_helper.h"
#include "bat_publishers.h"
#include "static_values.h"

#include "rapidjson_bat_helper.h"

using namespace braveledger_bat_client;
using namespace braveledger_bat_publishers;
using namespace braveledger_bat_get_media;
using namespace std::placeholders;

namespace bat_ledger {

LedgerImpl::LedgerImpl(ledger::LedgerClient* client) :
    ledger_client_(client),
    bat_client_(new BatClient(this)),
    bat_publishers_(new BatPublishers(this)),
    bat_get_media_(new BatGetMedia(this)) {
}

LedgerImpl::~LedgerImpl() {
}

void LedgerImpl::CreateWallet() {
  initSynopsis();  // fix race condition here
  LoadLedgerState(this);
}

void LedgerImpl::LoadLedgerState(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadLedgerState(handler);
}

void LedgerImpl::LoadPublisherState(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadPublisherState(handler);
}

void LedgerImpl::SaveLedgerState(const std::string& data,
                                 ledger::LedgerCallbackHandler* handler) {
  ledger_client_->SaveLedgerState(data, handler);
}

void LedgerImpl::SavePublisherState(const std::string& data,
                                    ledger::LedgerCallbackHandler* handler) {
  ledger_client_->SavePublisherState(data, handler);
}

std::string LedgerImpl::GenerateGUID() const {
  return ledger_client_->GenerateGUID();
}

void LedgerImpl::OnLedgerStateLoaded(ledger::Result result,
                                     const std::string& data) {
  bat_client_->loadStateOrRegisterPersonaCallback(result == ledger::Result::OK, data);
}

void LedgerImpl::OnWalletCreated(ledger::Result result) {
  ledger_client_->OnWalletCreated(result);
}

uint64_t LedgerImpl::LoadURL(const std::string& url,
                    const std::vector<std::string>& headers,
                    const std::string& content,
                    const std::string& contentType,
                    const ledger::URL_METHOD& method,
                    ledger::LedgerCallbackHandler* handler) {
  return ledger_client_->LoadURL(
      url, headers, content, contentType, method, handler);
}

void LedgerImpl::initSynopsis() {
  bat_publishers_->initSynopsis();
}

// void LedgerImpl::walletPropertiesCallback(bool success,
//                                           const std::string& response) {
//   if (!success) {
//     // TODO errors handling
//     return;
//   }
//   braveledger_bat_helper::WALLET_PROPERTIES_ST walletProperties;
//   braveledger_bat_helper::loadFromJson(walletProperties, response);
//   // TODO send the balance to the UI via observer or callback
// }

void LedgerImpl::SaveVisit(const std::string& publisher, uint64_t duration, bool ignoreMinTime) {
  // TODO debug
  //bat_client_->recoverWallet(bat_client_->getWalletPassphrase());
  //
  auto callback = std::bind(&LedgerImpl::saveVisitCallback, this, _1, _2);
  bat_publishers_->saveVisit(publisher, duration, callback, ignoreMinTime);
}

void LedgerImpl::RunIOTask(LedgerTaskRunnerImpl::Task io_task) {
  std::unique_ptr<LedgerTaskRunnerImpl> task_runner(
      new LedgerTaskRunnerImpl(io_task));
  ledger_client_->RunIOTask(std::move(task_runner));
}

void LedgerImpl::RunTask(LedgerTaskRunnerImpl::Task task) {
  std::unique_ptr<LedgerTaskRunnerImpl> task_runner(
      new LedgerTaskRunnerImpl(task));
  ledger_client_->RunTask(std::move(task_runner));
}

void LedgerImpl::saveVisitCallback(const std::string& publisher,
                                   uint64_t verifiedTimestamp) {
  uint64_t publisherTimestamp = bat_client_->getPublisherTimestamp();
  if (publisherTimestamp <= verifiedTimestamp) {
    //to do debug
    LOG(ERROR) << "!!!reconcile";
    Reconcile();
    //
    return;
  }

  // Update publisher verified or not flag
  //LOG(ERROR) << "!!!getting publisher info";
  auto request_id = bat_client_->publisherInfo(publisher, &handler_);
  handler_.AddRequestHandler(request_id, std::bind(&LedgerImpl::publisherInfoCallback,
                                            this,
                                            publisher,
                                            publisherTimestamp,
                                            _1,
                                            _2));
}

void LedgerImpl::publisherInfoCallback(const std::string& publisher,
                                       uint64_t publisher_timestamp,
                                       bool success,
                                       const std::string& response) {
  //LOG(ERROR) << "!!!got publisher info";
  if (!success) {
    // TODO errors handling
    return;
  }
  bool verified = false;
  braveledger_bat_helper::getJSONPublisherVerified(response, verified);
  bat_publishers_->setPublisherTimestampVerified(publisher, publisher_timestamp, verified);
  //to do debug
  //LOG(ERROR) << "!!!reconcile";
  //run(0);
  //
}

void LedgerImpl::SetPublisherInclude(const std::string& publisher, bool include) {
  bat_publishers_->setPublisherInclude(publisher, include);
}

void LedgerImpl::SetPublisherDeleted(const std::string& publisher, bool deleted) {
  bat_publishers_->setPublisherDeleted(publisher, deleted);
}

void LedgerImpl::SetPublisherPinPercentage(const std::string& publisher, bool pinPercentage) {
  bat_publishers_->setPublisherPinPercentage(publisher, pinPercentage);
}

void LedgerImpl::SetPublisherMinVisitTime(uint64_t duration) { // In milliseconds
  bat_publishers_->setPublisherMinVisitTime(duration);
}

void LedgerImpl::SetPublisherMinVisits(unsigned int visits) {
  bat_publishers_->setPublisherMinVisits(visits);
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  bat_publishers_->setPublisherAllowNonVerified(allow);
}

void LedgerImpl::SetContributionAmount(double amount) {
  bat_client_->setContributionAmount(amount);
}

const std::string& LedgerImpl::GetBATAddress() const {
  return bat_client_->getBATAddress();
}

const std::string& LedgerImpl::GetBTCAddress() const {
  return bat_client_->getBTCAddress();
}

const std::string& LedgerImpl::GetETHAddress() const {
  return bat_client_->getETHAddress();
}

const std::string& LedgerImpl::GetLTCAddress() const {
  return bat_client_->getLTCAddress();
}

void LedgerImpl::Reconcile() {
  // That function should be triggeres from the main process periodically to make payments
  if (bat_client_->isReadyForReconcile()) {
    bat_client_->reconcile(GenerateGUID());
  }
}

void LedgerImpl::OnReconcileComplete(ledger::Result result,
                                    const std::string& viewing_id) {
  ledger_client_->OnReconcileComplete(result, viewing_id);
  if (result != ledger::Result::OK) {
    // error handling
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
  bat_client_->votePublishers(publishers, viewing_id);
  // TODO call prepareBallots by timeouts like in js library
  bat_client_->prepareBallots();
}

void LedgerImpl::OnMediaRequest(const std::string& url,
                                const std::string& urlQuery,
                                const std::string& type) {
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

void LedgerImpl::processMedia(const std::map<std::string, std::string>& parts, const std::string& type) {
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

  braveledger_bat_helper::GetMediaPublisherInfoCallback callback = std::bind(&LedgerImpl::OnMediaRequestCallback, this, _1, _2);
  auto io_task = std::bind(&BatGetMedia::getPublisherFromMediaProps,
    bat_get_media_.get(), mediaId, mediaKey, type, duration, twitchEventInfo, callback);
  RunIOTask(io_task);
}

void LedgerImpl::OnMediaRequestCallback(uint64_t duration, const braveledger_bat_helper::MEDIA_PUBLISHER_INFO& mediaPublisherInfo) {
  SaveVisit(mediaPublisherInfo.publisher_, duration, true);
}

}  // namespace bat_ledger

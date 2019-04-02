/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>
#include <iostream>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

#include "base/task/post_task.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "bat/ads/issuers_info.h"
#include "bat/ads/notification_info.h"
#include "bat/confirmations/confirmations.h"
#include "bat/ledger/internal/bat_client.h"
#include "bat/ledger/internal/bat_contribution.h"
#include "bat/ledger/internal/bat_get_media.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/bat_publishers.h"
#include "bat/ledger/internal/bat_state.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"

using namespace braveledger_bat_client; //  NOLINT
using namespace braveledger_bat_publishers; //  NOLINT
using namespace braveledger_bat_get_media; //  NOLINT
using namespace braveledger_bat_get_media; //  NOLINT
using namespace braveledger_bat_state; //  NOLINT
using namespace braveledger_bat_contribution; //  NOLINT
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

namespace bat_ledger {

LedgerImpl::LedgerImpl(ledger::LedgerClient* client) :
    ledger_client_(client),
    bat_client_(new BatClient(this)),
    bat_publishers_(new BatPublishers(this)),
    bat_get_media_(new BatGetMedia(this)),
    bat_state_(new BatState(this)),
    bat_contribution_(new BatContribution(this)),
    initialized_task_scheduler_(false),
    initialized_(false),
    initializing_(false),
    last_tab_active_time_(0),
    last_shown_tab_id_(-1),
    last_pub_load_timer_id_(0u),
    last_grant_check_timer_id_(0u) {
  // Ensure TaskScheduler is initialized before creating the task runner for
  // ios.
  if (!base::TaskScheduler::GetInstance()) {
    base::TaskScheduler::CreateAndStartWithDefaultParams("bat_ledger");

    DCHECK(base::TaskScheduler::GetInstance());
    initialized_task_scheduler_ = true;
  }

  task_runner_ = base::CreateSequencedTaskRunnerWithTraits({
      base::MayBlock(), base::TaskPriority::BEST_EFFORT,
      base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
}

LedgerImpl::~LedgerImpl() {
  if (initialized_task_scheduler_) {
    DCHECK(base::TaskScheduler::GetInstance());
    base::TaskScheduler::GetInstance()->Shutdown();
  }
}

void LedgerImpl::Initialize() {
  DCHECK(!initializing_);
  initializing_ = true;
  LoadLedgerState(this);
}

bool LedgerImpl::CreateWallet() {
  if (initializing_) {
    return false;
  }

  initializing_ = true;
  bat_client_->CreateWalletIfNecessary();
  return true;
}

void LedgerImpl::AddRecurringPayment(const std::string& publisher_id,
                                     const double& value) {
  bat_publishers_->AddRecurringPayment(publisher_id, value);
}

braveledger_bat_helper::CURRENT_RECONCILE LedgerImpl::GetReconcileById(
    const std::string& viewingId) {
  return bat_state_->GetReconcileById(viewingId);
}

void LedgerImpl::RemoveReconcileById(const std::string& viewingId) {
  bat_state_->RemoveReconcileById(viewingId);
}

void LedgerImpl::OnLoad(const ledger::VisitData& visit_data,
                        const uint64_t& current_time) {
  if (visit_data.domain.empty()) {
    // Skip the same domain name
    return;
  }
  visit_data_iter iter = current_pages_.find(visit_data.tab_id);
  if (iter != current_pages_.end() &&
      iter->second.domain == visit_data.domain) {
    DCHECK(iter == current_pages_.end());
    return;
  }

  if (last_shown_tab_id_ == visit_data.tab_id) {
    last_tab_active_time_ = current_time;
  }
  current_pages_[visit_data.tab_id] = visit_data;
}

void LedgerImpl::OnUnload(uint32_t tab_id, const uint64_t& current_time) {
  OnHide(tab_id, current_time);
  visit_data_iter iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void LedgerImpl::OnShow(uint32_t tab_id, const uint64_t& current_time) {
  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void LedgerImpl::OnHide(uint32_t tab_id, const uint64_t& current_time) {
  if (tab_id != last_shown_tab_id_) {
    return;
  }
  visit_data_iter iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end() || last_tab_active_time_ == 0) {
    return;
  }
  DCHECK(last_tab_active_time_);
  bat_publishers_->saveVisit(
    iter->second.tld, iter->second, current_time - last_tab_active_time_, 0);
  last_tab_active_time_ = 0;
}

void LedgerImpl::OnForeground(uint32_t tab_id, const uint64_t& current_time) {
  // TODO(anyone) media resources could have been played in the background
  if (last_shown_tab_id_ != tab_id) {
    return;
  }
  OnShow(tab_id, current_time);
}

void LedgerImpl::OnBackground(uint32_t tab_id, const uint64_t& current_time) {
  // TODO(anyone) media resources could stay and be active in the background
  OnHide(tab_id, current_time);
}

void LedgerImpl::OnMediaStart(uint32_t tab_id, const uint64_t& current_time) {
  // TODO(anyone)
}

void LedgerImpl::OnMediaStop(uint32_t tab_id, const uint64_t& current_time) {
  // TODO(anyone)
}

void LedgerImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const std::map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    const ledger::VisitData& visit_data) {
  std::string type = bat_get_media_->GetLinkType(url,
                                                 first_party_url,
                                                 referrer);
  if (type.empty()) {
    // It is not a media supported type
    return;
  }
  bat_get_media_->processMedia(parts, type, visit_data);
}

void LedgerImpl::OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      const ledger::VisitData& visit_data) {
  std::string type = bat_get_media_->GetLinkType(url,
                                                 first_party_url,
                                                 referrer);
  if (type.empty()) {
     // It is not a media supported type
    return;
  }

  std::vector<std::map<std::string, std::string>> twitchParts;
  if (TWITCH_MEDIA_TYPE == type) {
    braveledger_bat_helper::getTwitchParts(post_data, &twitchParts);
    for (size_t i = 0; i < twitchParts.size(); i++) {
      bat_get_media_->processMedia(twitchParts[i], type, visit_data);
    }
  }
}

void LedgerImpl::LoadLedgerState(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadLedgerState(handler);
}

void LedgerImpl::OnLedgerStateLoaded(ledger::Result result,
                                     const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_state_->LoadState(data)) {
      BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Successfully loaded but failed to parse ledger state.";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed ledger state: " << data;

      OnWalletInitialized(ledger::Result::INVALID_LEDGER_STATE);
    } else {
      auto wallet_info = bat_state_->GetWalletInfo();
      SetConfirmationsWalletInfo(wallet_info);

      LoadPublisherState(this);
      bat_contribution_->OnStartUp();
    }
  } else {
    if (result != ledger::Result::NO_LEDGER_STATE) {
      BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to load ledger state";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed ledger state: " <<
        data;
    }

    OnWalletInitialized(result);
  }
}

void LedgerImpl::SetConfirmationsWalletInfo(
    const braveledger_bat_helper::WALLET_INFO_ST& wallet_info) {
  if (!bat_confirmations_) {
    confirmations::_is_production = ledger::is_production;
    confirmations::_is_debug = ledger::is_debug;

    bat_confirmations_.reset(
        confirmations::Confirmations::CreateInstance(ledger_client_));
    bat_confirmations_->Initialize();
  }

  auto confirmations_wallet_info = GetConfirmationsWalletInfo(wallet_info);
  bat_confirmations_->SetWalletInfo(
      std::make_unique<confirmations::WalletInfo>(confirmations_wallet_info));
}

void LedgerImpl::LoadPublisherState(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadPublisherState(handler);
}

void LedgerImpl::OnPublisherStateLoaded(ledger::Result result,
                                        const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_publishers_->loadState(data)) {
      BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Successfully loaded but failed to parse ledger state.";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed publisher state: " << data;

      result = ledger::Result::INVALID_PUBLISHER_STATE;
    }
  } else {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed to load publisher state";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed publisher state: " << data;
  }

  OnWalletInitialized(result);
}

void LedgerImpl::SaveLedgerState(const std::string& data) {
  ledger_client_->SaveLedgerState(data, this);
}

void LedgerImpl::SavePublisherState(const std::string& data,
                                    ledger::LedgerCallbackHandler* handler) {
  ledger_client_->SavePublisherState(data, handler);
}


void LedgerImpl::SavePublishersList(const std::string& data) {
  ledger_client_->SavePublishersList(data, this);
}

void LedgerImpl::LoadPublisherList(ledger::LedgerCallbackHandler* handler) {
  ledger_client_->LoadPublisherList(handler);
}

void LedgerImpl::OnPublisherListLoaded(ledger::Result result,
                                       const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_publishers_->loadPublisherList(data)) {
      BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Successfully loaded but failed to parse publish list.";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed publisher list: " << data;
      RefreshPublishersList(true);
      return;
    } else {
      // List was loaded successfully
      RefreshPublishersList(false);
      return;
    }
  } else {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed to load publisher list";
    BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
      "Failed publisher list: " << data;
  }

  RefreshPublishersList(true, true);
}

std::string LedgerImpl::GenerateGUID() const {
  return ledger_client_->GenerateGUID();
}

void LedgerImpl::OnWalletInitialized(ledger::Result result) {
  initializing_ = false;
  ledger_client_->OnWalletInitialized(result);

  if (result == ledger::Result::LEDGER_OK ||
      result == ledger::Result::WALLET_CREATED) {
    initialized_ = true;
    LoadPublisherList(this);
    bat_contribution_->SetReconcileTimer();
    RefreshGrant(false);
  } else {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to initialize wallet";
  }
}

void LedgerImpl::LoadURL(const std::string& url,
                         const std::vector<std::string>& headers,
                         const std::string& content,
                         const std::string& contentType,
                         const ledger::URL_METHOD method,
                         ledger::LoadURLCallback callback) {
  ledger_client_->LoadURL(url,
                          headers,
                          content,
                          contentType,
                          method,
                          callback);
}

std::string LedgerImpl::URIEncode(const std::string& value) {
  return ledger_client_->URIEncode(value);
}

void LedgerImpl::OnPublisherInfoSavedInternal(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  bat_publishers_->OnPublisherInfoSaved(result, std::move(info));
}

void LedgerImpl::SetPublisherInfo(std::unique_ptr<ledger::PublisherInfo> info) {
  ledger_client_->SavePublisherInfo(
      std::move(info),
      std::bind(&LedgerImpl::OnPublisherInfoSavedInternal,
                this,
                _1,
                _2));
}

void LedgerImpl::SetActivityInfo(std::unique_ptr<ledger::PublisherInfo> info) {
  ledger_client_->SaveActivityInfo(
      std::move(info),
      std::bind(&LedgerImpl::OnPublisherInfoSavedInternal,
                this,
                _1,
                _2));
}

void LedgerImpl::SetMediaPublisherInfo(const std::string& media_key,
                                       const std::string& publisher_id) {
  if (!media_key.empty() && !publisher_id.empty()) {
    ledger_client_->SaveMediaPublisherInfo(media_key, publisher_id);
  }
}

void LedgerImpl::SaveMediaVisit(const std::string& publisher_id,
                                const ledger::VisitData& visit_data,
                                const uint64_t& duration,
                                const uint64_t window_id) {
  uint64_t new_duration = duration;
  if (!bat_publishers_->getPublisherAllowVideos()) {
    new_duration = 0;
  }

  bat_publishers_->saveVisit(publisher_id, visit_data, new_duration, window_id);
}

void LedgerImpl::SetPublisherExclude(
    const std::string& publisher_id,
    const ledger::PUBLISHER_EXCLUDE& exclude) {
  bat_publishers_->setExclude(publisher_id, exclude);
}

void LedgerImpl::RestorePublishers() {
  bat_publishers_->RestorePublishers();
}

void LedgerImpl::OnRestorePublishers(ledger::OnRestoreCallback callback) {
  ledger_client_->OnRestorePublishers(callback);
}

void LedgerImpl::LoadNicewareList(ledger::GetNicewareListCallback callback) {
  ledger_client_->LoadNicewareList(callback);
}

std::vector<ledger::ContributionInfo>
LedgerImpl::GetRecurringDonationPublisherInfo() {
  return bat_publishers_->GetRecurringDonationList();
}

void LedgerImpl::GetPublisherInfo(const std::string& publisher_key,
                                  ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadPublisherInfo(publisher_key, callback);
}

void LedgerImpl::GetActivityInfo(const ledger::ActivityInfoFilter& filter,
                                 ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadActivityInfo(filter, callback);
}

void LedgerImpl::GetPanelPublisherInfo(
    const ledger::ActivityInfoFilter& filter,
    ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadPanelPublisherInfo(filter, callback);
}

void LedgerImpl::GetMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadMediaPublisherInfo(media_key, callback);
}

void LedgerImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    const ledger::ActivityInfoFilter& filter,
    ledger::PublisherInfoListCallback callback) {
  ledger_client_->GetActivityInfoList(start, limit, filter, callback);
}

void LedgerImpl::SetRewardsMainEnabled(bool enabled) {
  bat_state_->SetRewardsMainEnabled(enabled);
}

void LedgerImpl::SetPublisherMinVisitTime(uint64_t duration) {  // In seconds
  bat_publishers_->setPublisherMinVisitTime(duration);
}

void LedgerImpl::SetPublisherMinVisits(unsigned int visits) {
  bat_publishers_->setPublisherMinVisits(visits);
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  bat_publishers_->setPublisherAllowNonVerified(allow);
}

void LedgerImpl::SetPublisherAllowVideos(bool allow) {
  bat_publishers_->setPublisherAllowVideos(allow);
}

void LedgerImpl::SetContributionAmount(double amount) {
  bat_state_->SetContributionAmount(amount);
}

void LedgerImpl::SetUserChangedContribution() {
  bat_state_->SetUserChangedContribution();
}

bool LedgerImpl::GetUserChangedContribution() {
  return bat_state_->GetUserChangedContribution();
}

void LedgerImpl::SetAutoContribute(bool enabled) {
  bat_state_->SetAutoContribute(enabled);
}

void LedgerImpl::GetAutoContributeProps(ledger::AutoContributeProps* props) {
  props->enabled_contribute = GetAutoContribute();
  props->contribution_min_time = GetPublisherMinVisitTime();
  props->contribution_min_visits = GetPublisherMinVisits();
  props->contribution_non_verified = GetPublisherAllowNonVerified();
  props->contribution_videos = GetPublisherAllowVideos();
  props->reconcile_stamp = GetReconcileStamp();
}

bool LedgerImpl::GetRewardsMainEnabled() const {
  return bat_state_->GetRewardsMainEnabled();
}

uint64_t LedgerImpl::GetPublisherMinVisitTime() const {
  return bat_publishers_->getPublisherMinVisitTime();
}

unsigned int LedgerImpl::GetPublisherMinVisits() const {
  return bat_publishers_->getPublisherMinVisits();
}

void LedgerImpl::GetExcludedPublishersNumber(
    ledger::GetExcludedPublishersNumberDBCallback callback) const {
  ledger_client_->GetExcludedPublishersNumberDB(callback);
}

bool LedgerImpl::GetPublisherAllowNonVerified() const {
  return bat_publishers_->getPublisherAllowNonVerified();
}

bool LedgerImpl::GetPublisherAllowVideos() const {
  return bat_publishers_->getPublisherAllowVideos();
}

double LedgerImpl::GetContributionAmount() const {
  return bat_state_->GetContributionAmount();
}

bool LedgerImpl::GetAutoContribute() const {
  return bat_state_->GetAutoContribute();
}

std::map<std::string, std::string> LedgerImpl::GetAddresses() {
  std::map<std::string, std::string> addresses;
  addresses.emplace("BAT", GetBATAddress());
  addresses.emplace("BTC", GetBTCAddress());
  addresses.emplace("ETH", GetETHAddress());
  addresses.emplace("LTC", GetLTCAddress());
  return addresses;
}

const std::string& LedgerImpl::GetBATAddress() const {
  return bat_state_->GetBATAddress();
}

const std::string& LedgerImpl::GetBTCAddress() const {
  return bat_state_->GetBTCAddress();
}

const std::string& LedgerImpl::GetETHAddress() const {
  return bat_state_->GetETHAddress();
}

const std::string& LedgerImpl::GetLTCAddress() const {
  return bat_state_->GetLTCAddress();
}

uint64_t LedgerImpl::GetReconcileStamp() const {
  return bat_state_->GetReconcileStamp();
}

void LedgerImpl::OnReconcileComplete(ledger::Result result,
                                     const std::string& viewing_id,
                                     const std::string& probi) {
  auto reconcile = GetReconcileById(viewing_id);

  ledger_client_->OnReconcileComplete(
      result,
      viewing_id,
      (ledger::REWARDS_CATEGORY)reconcile.category_,
      probi);
}

void LedgerImpl::OnWalletProperties(
    ledger::Result result,
    const braveledger_bat_helper::WALLET_PROPERTIES_ST& properties) {
  std::unique_ptr<ledger::WalletInfo> info;

  if (result == ledger::Result::LEDGER_OK) {
    info.reset(new ledger::WalletInfo(
        bat_client_->WalletPropertiesToWalletInfo(properties)));
  }

  ledger_client_->OnWalletProperties(result, std::move(info));
}

void LedgerImpl::FetchWalletProperties(
    ledger::OnWalletPropertiesCallback callback) const {
  bat_client_->GetWalletProperties(callback);
}

void LedgerImpl::FetchGrants(const std::string& lang,
                             const std::string& payment_id) const {
  bat_client_->getGrants(lang, payment_id);
}

void LedgerImpl::OnGrant(ledger::Result result,
                         const braveledger_bat_helper::GRANT& properties) {
  ledger::Grant grant;

  grant.type = properties.type;
  grant.promotionId = properties.promotionId;
  last_grant_check_timer_id_ = 0;

  RefreshGrant(result != ledger::Result::LEDGER_OK &&
    result != ledger::Result::GRANT_NOT_FOUND);
  ledger_client_->OnGrant(result, grant);
}

void LedgerImpl::GetGrantCaptcha(
    const std::string& promotion_id,
    const std::string& promotion_type) const {
  bat_client_->getGrantCaptcha(promotion_id, promotion_type);
}

void LedgerImpl::OnGrantCaptcha(const std::string& image,
                                const std::string& hint) {
  ledger_client_->OnGrantCaptcha(image, hint);
}

std::string LedgerImpl::GetWalletPassphrase() const {
  return bat_client_->getWalletPassphrase();
}

void LedgerImpl::RecoverWallet(const std::string& passPhrase) const {
  bat_client_->recoverWallet(passPhrase);
}

void LedgerImpl::OnRecoverWallet(
    ledger::Result result,
    double balance,
    const std::vector<braveledger_bat_helper::GRANT>& grants) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to recover wallet";
  }

  std::vector<ledger::Grant> ledgerGrants;

  for (size_t i = 0; i < grants.size(); i ++) {
    ledger::Grant tempGrant;

    tempGrant.altcurrency = grants[i].altcurrency;
    tempGrant.probi = grants[i].probi;
    tempGrant.expiryTime = grants[i].expiryTime;

    ledgerGrants.push_back(tempGrant);
  }
  if (result == ledger::Result::LEDGER_OK) {
    bat_publishers_->clearAllBalanceReports();
  }

  ledger_client_->OnRecoverWallet(result ? ledger::Result::LEDGER_ERROR :
                                          ledger::Result::LEDGER_OK,
                                  balance,
                                  ledgerGrants);
}

void LedgerImpl::SolveGrantCaptcha(
    const std::string& solution,
    const std::string& promotionId) const {
  bat_client_->setGrant(solution, promotionId);
}

void LedgerImpl::OnGrantFinish(ledger::Result result,
                               const braveledger_bat_helper::GRANT& grant) {
  ledger::Grant newGrant;

  newGrant.altcurrency = grant.altcurrency;
  newGrant.probi = grant.probi;
  newGrant.expiryTime = grant.expiryTime;
  newGrant.promotionId = grant.promotionId;
  newGrant.type = grant.type;

  ledger_client_->OnGrantFinish(result, newGrant);
}

bool LedgerImpl::GetBalanceReport(
    ledger::ACTIVITY_MONTH month,
    int year,
    ledger::BalanceReportInfo* report_info) const {
  return bat_publishers_->getBalanceReport(month, year, report_info);
}

std::map<std::string, ledger::BalanceReportInfo>
LedgerImpl::GetAllBalanceReports() const {
  return bat_publishers_->getAllBalanceReports();
}

void LedgerImpl::SetBalanceReport(
    ledger::ACTIVITY_MONTH month,
    int year,
    const ledger::BalanceReportInfo& report_info) {
  bat_publishers_->setBalanceReport(month, year, report_info);
}

void LedgerImpl::SaveUnverifiedContribution(
    const ledger::PendingContributionList& list) {
  ledger_client_->SavePendingContribution(list);
}

void LedgerImpl::DoDirectDonation(const ledger::PublisherInfo& publisher,
                                  int amount,
                                  const std::string& currency) {
  if (publisher.id.empty()) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed direct donation due to missing publisher id";

    // TODO(anyone) add error flow
    return;
  }

  bool is_verified = bat_publishers_->isVerified(publisher.id);

  // Save to the pending list if not verified
  if (!is_verified) {
    ledger::PendingContribution contribution;
    contribution.publisher_key = publisher.id;
    contribution.amount = amount;
    contribution.category = ledger::REWARDS_CATEGORY::ONE_TIME_TIP;

    ledger::PendingContributionList list;
    list.list_ = std::vector<ledger::PendingContribution> { contribution };

    SaveUnverifiedContribution(list);

    return;
  }

  auto direction = braveledger_bat_helper::RECONCILE_DIRECTION(publisher.id,
                                                               amount,
                                                               currency);
  auto direction_list =
      std::vector<braveledger_bat_helper::RECONCILE_DIRECTION> { direction };
  braveledger_bat_helper::PublisherList list;
  bat_contribution_->InitReconcile(GenerateGUID(),
                         ledger::REWARDS_CATEGORY::ONE_TIME_TIP,
                         list,
                         direction_list);
}

void LedgerImpl::OnTimer(uint32_t timer_id) {
  if (bat_confirmations_->OnTimer(timer_id))
    return;

  if (timer_id == last_pub_load_timer_id_) {
    last_pub_load_timer_id_ = 0;

    std::vector<std::string> headers;
    headers.push_back("Accept-Encoding: gzip");

    // download the list
    std::string url = braveledger_bat_helper::buildURL(
        GET_PUBLISHERS_LIST_V1,
        "",
        braveledger_bat_helper::SERVER_TYPES::PUBLISHER_DISTRO);
    auto callback = std::bind(&LedgerImpl::LoadPublishersListCallback,
                              this,
                              _1,
                              _2,
                              _3);
    LoadURL(url, headers, "", "", ledger::URL_METHOD::GET, callback);
  } else if (timer_id == last_grant_check_timer_id_) {
    last_grant_check_timer_id_ = 0;
    FetchGrants(std::string(), std::string());
  }

  bat_contribution_->OnTimer(timer_id);
}

void LedgerImpl::GetRecurringTips(
    ledger::PublisherInfoListCallback callback) {
  ledger_client_->GetRecurringTips(callback);
}

void LedgerImpl::LoadPublishersListCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code == 200 && !response.empty()) {
    bat_publishers_->RefreshPublishersList(response);
  } else {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Can't fetch publisher list";
    // error: retry downloading again
    RefreshPublishersList(true);
  }
}

void LedgerImpl::RefreshPublishersList(bool retryAfterError, bool immediately) {
  uint64_t start_timer_in{ 0ull };

  if (last_pub_load_timer_id_ != 0) {
    // timer in progress
    return;
  }

  uint64_t lastLoadTimestamp =
        bat_publishers_->getLastPublishersListLoadTimestamp();

  if (immediately) {
    start_timer_in = 0ull;
  } else if (retryAfterError) {
    start_timer_in = retryRequestSetup(60, 300);

    BLOG(this, ledger::LogLevel::LOG_WARNING) <<
      "Failed to refresh publishesr list, will try again in " <<
      start_timer_in << " seconds.";
  } else {
    uint64_t now = std::time(nullptr);

    // check if lastLoadTimestamp doesn't exist or have erroneous value.
    // (start_timer_in == 0) is expected to call callback function immediately.

    // time since last successful download
    uint64_t  time_since_last_download =
        (lastLoadTimestamp == 0ull || lastLoadTimestamp > now)
        ? 0ull
        : now - lastLoadTimestamp;

    uint64_t interval = braveledger_ledger::_publishers_list_load_interval;

    if (now == lastLoadTimestamp) {
      start_timer_in = interval;
    } else if (time_since_last_download > 0 &&
               time_since_last_download < interval) {
      start_timer_in = interval - time_since_last_download;
    } else {
      start_timer_in = 0ull;
    }
  }

  // start timer
  SetTimer(start_timer_in, &last_pub_load_timer_id_);
}

void LedgerImpl::RefreshGrant(bool retryAfterError) {
  uint64_t start_timer_in{ 0ull };
  if (last_grant_check_timer_id_ != 0) {
    return;
  }

  if (retryAfterError) {
    start_timer_in = retryRequestSetup(300, 600);

    BLOG(this, ledger::LogLevel::LOG_WARNING) <<
      "Failed to refresh grant, will try again in " << start_timer_in;
  } else {
    uint64_t now = std::time(nullptr);
    uint64_t last_grant_stamp = bat_state_->GetLastGrantLoadTimestamp();

    uint64_t time_since_last_grant_check = (last_grant_stamp == 0ull ||
      last_grant_stamp > now) ? 0ull : now - last_grant_stamp;
    if (now == last_grant_stamp) {
      start_timer_in = braveledger_ledger::_grant_load_interval;
    } else if (time_since_last_grant_check > 0 &&
      time_since_last_grant_check < braveledger_ledger::_grant_load_interval) {
      start_timer_in =
        braveledger_ledger::_grant_load_interval - time_since_last_grant_check;
    } else {
      start_timer_in = 0ull;
    }
  }
  SetTimer(start_timer_in, &last_grant_check_timer_id_);
}

uint64_t LedgerImpl::retryRequestSetup(uint64_t min_time, uint64_t max_time) {
  std::random_device seeder;
  const auto seed = seeder.entropy() ? seeder() : time(nullptr);
  std::mt19937 eng(static_cast<std::mt19937::result_type>(seed));
  DCHECK(max_time > min_time);
  std::uniform_int_distribution <> dist(min_time, max_time);
  return dist(eng);
}

void LedgerImpl::OnPublishersListSaved(ledger::Result result) {
  bool retryAfterError = !(ledger::Result::LEDGER_OK == result);
  bat_publishers_->OnPublishersListSaved(result);
  RefreshPublishersList(retryAfterError);
}

bool LedgerImpl::IsWalletCreated() const {
  return bat_state_->IsWalletCreated();
}

void LedgerImpl::GetPublisherActivityFromUrl(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& publisher_blob) {
  bat_publishers_->getPublisherActivityFromUrl(
      windowId,
      visit_data,
      publisher_blob);
}

void LedgerImpl::GetMediaActivityFromUrl(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_blob) {
  bat_get_media_->getMediaActivityFromUrl(
      windowId, visit_data, providerType, publisher_blob);
}

void LedgerImpl::OnPanelPublisherInfo(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId) {
  ledger_client_->OnPanelPublisherInfo(result, std::move(info), windowId);
}

void LedgerImpl::OnExcludedSitesChanged(const std::string& publisher_id,
                                        ledger::PUBLISHER_EXCLUDE exclude) {
  ledger_client_->OnExcludedSitesChanged(publisher_id, exclude);
}

void LedgerImpl::SetBalanceReportItem(ledger::ACTIVITY_MONTH month,
                                      int year,
                                      ledger::ReportType type,
                                      const std::string& probi) {
  bat_publishers_->setBalanceReportItem(month, year, type, probi);
}

void LedgerImpl::FetchFavIcon(const std::string& url,
                              const std::string& favicon_key,
                              ledger::FetchIconCallback callback) {
  ledger_client_->FetchFavIcon(url, favicon_key, callback);
}

void LedgerImpl::GetPublisherBanner(const std::string& publisher_id,
                                    ledger::PublisherBannerCallback callback) {
  bat_publishers_->getPublisherBanner(publisher_id, callback);
}

double LedgerImpl::GetBalance() {
  return bat_state_->GetBalance();
}

void LedgerImpl::OnReconcileCompleteSuccess(
    const std::string& viewing_id,
    const ledger::REWARDS_CATEGORY category,
    const std::string& probi,
    const ledger::ACTIVITY_MONTH month,
    const int year,
    const uint32_t date) {
  bat_contribution_->OnReconcileCompleteSuccess(viewing_id,
                                                category,
                                                probi,
                                                month,
                                                year,
                                                date);
}

void LedgerImpl::RemoveRecurringTip(const std::string& publisher_key) {
  ledger_client_->OnRemoveRecurring(
      publisher_key,
      std::bind(&LedgerImpl::OnRemovedRecurring,
                this,
                _1));
}

void LedgerImpl::OnRemovedRecurring(ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed to remove recurring";

    // TODO(anyone) add error callback
    return;
  }
}

ledger::ActivityInfoFilter LedgerImpl::CreateActivityFilter(
    const std::string& publisher_id,
    ledger::EXCLUDE_FILTER excluded,
    bool min_duration,
    const uint64_t& currentReconcileStamp,
    bool non_verified,
    bool min_visits) {
  return bat_publishers_->CreateActivityFilter(publisher_id,
                                               excluded,
                                               min_duration,
                                               currentReconcileStamp,
                                               non_verified,
                                               min_visits);
}


std::unique_ptr<ledger::LogStream> LedgerImpl::Log(
    const char* file,
    int line,
    const ledger::LogLevel log_level) const {
  // TODO(Terry Mancey): bat-native-ledger architecture does not expose the
  // client however the ledger impl is exposed so for now we will proxy logging
  // via from the ledger impl to the client
  return ledger_client_->Log(file, line, log_level);
}

void LedgerImpl::LogResponse(
    const std::string& func_name,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  std::string stat = response_status_code == 200 ? "Success" : "Failure";

  std::string formatted_headers = "";
  for (auto header = headers.begin(); header != headers.end(); ++header) {
    formatted_headers += "> headers " + header->first + ": " + header->second;
    if (header != headers.end()) {
      formatted_headers += "\n";
    }
  }

  BLOG(this, ledger::LogLevel::LOG_RESPONSE) << std::endl
    << "[ RESPONSE - " << func_name << " ]" << std::endl
    << "> time: " << std::time(nullptr) << std::endl
    << "> result: " << stat << std::endl
    << "> response: " << response
    << formatted_headers
    << "[ END RESPONSE ]";
}

void LedgerImpl::ResetReconcileStamp() {
  bat_state_->ResetReconcileStamp();
}

bool LedgerImpl::UpdateReconcile(
    const braveledger_bat_helper::CURRENT_RECONCILE& reconcile) {
  return bat_state_->UpdateReconcile(reconcile);
}

void LedgerImpl::AddReconcile(
      const std::string& viewing_id,
      const braveledger_bat_helper::CURRENT_RECONCILE& reconcile) {
  bat_state_->AddReconcile(viewing_id, reconcile);
}

const std::string& LedgerImpl::GetPaymentId() const {
  return bat_state_->GetPaymentId();
}

void LedgerImpl::SetPaymentId(const std::string& payment_id) {
  bat_state_->SetPaymentId(payment_id);
}

const braveledger_bat_helper::Grants& LedgerImpl::GetGrants() const {
  return bat_state_->GetGrants();
}

void LedgerImpl::SetGrants(braveledger_bat_helper::Grants grants) {
  bat_state_->SetGrants(grants);
}

const std::string& LedgerImpl::GetPersonaId() const {
  return bat_state_->GetPersonaId();
}

void LedgerImpl::SetPersonaId(const std::string& persona_id) {
  bat_state_->SetPersonaId(persona_id);
}

const std::string& LedgerImpl::GetUserId() const {
  return bat_state_->GetUserId();
}

void LedgerImpl::SetUserId(const std::string& user_id) {
  bat_state_->SetUserId(user_id);
}

const std::string& LedgerImpl::GetRegistrarVK() const {
  return bat_state_->GetRegistrarVK();
}

void LedgerImpl::SetRegistrarVK(const std::string& registrar_vk) {
  bat_state_->SetRegistrarVK(registrar_vk);
}

const std::string& LedgerImpl::GetPreFlight() const {
  return bat_state_->GetPreFlight();
}

void LedgerImpl::SetPreFlight(const std::string& pre_flight) {
  bat_state_->SetPreFlight(pre_flight);
}

const braveledger_bat_helper::WALLET_INFO_ST&
LedgerImpl::GetWalletInfo() const {
  return bat_state_->GetWalletInfo();
}

void LedgerImpl::SetWalletInfo(
    const braveledger_bat_helper::WALLET_INFO_ST& info) {
  bat_state_->SetWalletInfo(info);

  SetConfirmationsWalletInfo(info);
}

const confirmations::WalletInfo LedgerImpl::GetConfirmationsWalletInfo(
    const braveledger_bat_helper::WALLET_INFO_ST& info) const {
  confirmations::WalletInfo wallet_info;

  wallet_info.payment_id = info.paymentId_;

  if (info.keyInfoSeed_.empty()) {
    return wallet_info;
  }

  auto seed = braveledger_bat_helper::getHKDF(info.keyInfoSeed_);
  std::vector<uint8_t> publicKey = {};
  std::vector<uint8_t> secretKey = {};
  braveledger_bat_helper::getPublicKeyFromSeed(seed, &publicKey, &secretKey);

  wallet_info.public_key = braveledger_bat_helper::uint8ToHex(secretKey);

  return wallet_info;
}

void LedgerImpl::GetRewardsInternalsInfo(ledger::RewardsInternalsInfo* info) {
  // Retrieve the payment id.
  info->payment_id = bat_state_->GetPaymentId();

  // Retrieve the key info seed and validate it.
  const braveledger_bat_helper::WALLET_INFO_ST wallet_info =
      bat_state_->GetWalletInfo();
  if (wallet_info.keyInfoSeed_.size() != SEED_LENGTH) {
    info->is_key_info_seed_valid = false;
  } else {
    std::vector<uint8_t> secret_key =
        braveledger_bat_helper::getHKDF(wallet_info.keyInfoSeed_);
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> new_secret_key;
    info->is_key_info_seed_valid = braveledger_bat_helper::getPublicKeyFromSeed(
        secret_key, &public_key, &new_secret_key);
  }

  // Retrieve the current reconciles.
  const braveledger_bat_helper::CurrentReconciles current_reconciles =
      GetCurrentReconciles();
  for (const auto& reconcile : current_reconciles) {
    ledger::ReconcileInfo reconcile_info;
    reconcile_info.viewingId_ = reconcile.second.viewingId_;
    reconcile_info.amount_ = reconcile.second.amount_;
    reconcile_info.retry_step_ = reconcile.second.retry_step_;
    reconcile_info.retry_level_ = reconcile.second.retry_level_;
    info->current_reconciles.insert(
        std::make_pair(reconcile.second.viewingId_, reconcile_info));
  }
}

const braveledger_bat_helper::WALLET_PROPERTIES_ST&
LedgerImpl::GetWalletProperties() const {
  return bat_state_->GetWalletProperties();
}

void LedgerImpl::SetWalletProperties(
    braveledger_bat_helper::WALLET_PROPERTIES_ST* properties) {
  bat_state_->SetWalletProperties(properties);
}

unsigned int LedgerImpl::GetDays() const {
  return bat_state_->GetDays();
}

void LedgerImpl::SetDays(unsigned int days) {
  bat_state_->SetDays(days);
}

const braveledger_bat_helper::Transactions&
LedgerImpl::GetTransactions() const {
  return bat_state_->GetTransactions();
}

void LedgerImpl::SetTransactions(
    const braveledger_bat_helper::Transactions& transactions) {
  bat_state_->SetTransactions(transactions);
}

const braveledger_bat_helper::Ballots& LedgerImpl::GetBallots() const {
  return bat_state_->GetBallots();
}

void LedgerImpl::SetBallots(const braveledger_bat_helper::Ballots& ballots) {
  bat_state_->SetBallots(ballots);
}

const braveledger_bat_helper::BatchVotes& LedgerImpl::GetBatch() const {
  return bat_state_->GetBatch();
}

void LedgerImpl::SetBatch(const braveledger_bat_helper::BatchVotes& votes) {
  bat_state_->SetBatch(votes);
}

const std::string& LedgerImpl::GetCurrency() const {
  return bat_state_->GetCurrency();
}

void LedgerImpl::SetCurrency(const std::string& currency) {
  bat_state_->SetCurrency(currency);
}

void LedgerImpl::SetLastGrantLoadTimestamp(uint64_t stamp) {
  bat_state_->SetLastGrantLoadTimestamp(stamp);
}

uint64_t LedgerImpl::GetBootStamp() const {
  return bat_state_->GetBootStamp();
}

void LedgerImpl::SetBootStamp(uint64_t stamp) {
  bat_state_->SetBootStamp(stamp);
}

const std::string& LedgerImpl::GetMasterUserToken() const {
  return bat_state_->GetMasterUserToken();
}

void LedgerImpl::SetMasterUserToken(const std::string& token) {
  bat_state_->SetMasterUserToken(token);
}

bool LedgerImpl::ReconcileExists(const std::string& viewingId) {
  return bat_state_->ReconcileExists(viewingId);
}

void LedgerImpl::SaveContributionInfo(
    const std::string& probi,
    const int month,
    const int year,
    const uint32_t date,
    const std::string& publisher_key,
    const ledger::REWARDS_CATEGORY category) {
  ledger_client_->SaveContributionInfo(probi,
                                       month,
                                       year,
                                       date,
                                       publisher_key,
                                       category);
}

void LedgerImpl::NormalizeContributeWinners(
    ledger::PublisherInfoList* newList,
    const ledger::PublisherInfoList& list,
    uint32_t record) {
  bat_publishers_->NormalizeContributeWinners(newList, list, record);
}

void LedgerImpl::SetTimer(uint64_t time_offset, uint32_t* timer_id) const {
  ledger_client_->SetTimer(time_offset, timer_id);
}

bool LedgerImpl::AddReconcileStep(
    const std::string& viewing_id,
    ledger::ContributionRetry step,
    int level) {
  BLOG(this, ledger::LogLevel::LOG_DEBUG)
    << "Contribution step"
    << std::to_string(step)
    << "for"
    << viewing_id;
  return bat_state_->AddReconcileStep(viewing_id, step, level);
}

const braveledger_bat_helper::CurrentReconciles&
LedgerImpl::GetCurrentReconciles() const {
  return bat_state_->GetCurrentReconciles();
}

double LedgerImpl::GetDefaultContributionAmount() {
  return bat_state_->GetDefaultContributionAmount();
}

bool LedgerImpl::HasSufficientBalanceToReconcile() {
  return GetBalance() >= GetContributionAmount();
}

void LedgerImpl::SaveNormalizedPublisherList(
    const ledger::PublisherInfoList& normalized_list) {
  ledger::PublisherInfoListStruct list;
  list.list = normalized_list;
  ledger_client_->SaveNormalizedPublisherList(list);
}

void LedgerImpl::GetAddressesForPaymentId(
    ledger::WalletAddressesCallback callback) {
  bat_client_->GetAddressesForPaymentId(callback);
}

void LedgerImpl::SetAddresses(std::map<std::string, std::string> addresses) {
  bat_state_->SetAddress(addresses);
}

void LedgerImpl::SetCatalogIssuers(const std::string& info) {
  ads::IssuersInfo issuers_info_ads;
  if (issuers_info_ads.FromJson(info) != ads::Result::SUCCESS)
    return;

  auto issuers_info = std::make_unique<confirmations::IssuersInfo>();
  issuers_info->public_key = issuers_info_ads.public_key;
  for (ads::IssuerInfo issuer_info_ad : issuers_info_ads.issuers) {
    confirmations::IssuerInfo issuer_info;
    issuer_info.name = issuer_info_ad.name;
    issuer_info.public_key = issuer_info_ad.public_key;
    issuers_info->issuers.push_back(issuer_info);
  }

  bat_confirmations_->SetCatalogIssuers(std::move(issuers_info));
}

void LedgerImpl::ConfirmAd(const std::string& info) {
  ads::NotificationInfo notification_info_ads;
  if (notification_info_ads.FromJson(info) != ads::Result::SUCCESS)
    return;

  auto notification_info = std::make_unique<confirmations::NotificationInfo>();
  notification_info->creative_set_id = notification_info_ads.creative_set_id;
  notification_info->category = notification_info_ads.category;
  notification_info->advertiser = notification_info_ads.advertiser;
  notification_info->text = notification_info_ads.text;
  notification_info->url = notification_info_ads.url;
  notification_info->uuid = notification_info_ads.uuid;

  switch (notification_info_ads.type.value()) {
    case ads::ConfirmationType::UNKNOWN: {
      notification_info->type = confirmations::ConfirmationType::UNKNOWN;
      break;
    }

    case ads::ConfirmationType::CLICK: {
      notification_info->type = confirmations::ConfirmationType::CLICK;
      break;
    }

    case ads::ConfirmationType::DISMISS: {
      notification_info->type = confirmations::ConfirmationType::DISMISS;
      break;
    }

    case ads::ConfirmationType::VIEW: {
      notification_info->type = confirmations::ConfirmationType::VIEW;
      break;
    }

    case ads::ConfirmationType::LANDED: {
      notification_info->type = confirmations::ConfirmationType::LANDED;
      break;
    }
  }

  bat_confirmations_->ConfirmAd(std::move(notification_info));
}

void LedgerImpl::GetConfirmationsHistory(
    const uint64_t from_timestamp_seconds,
    const uint64_t to_timestamp_seconds,
    ledger::ConfirmationsHistoryCallback callback) {
  bat_confirmations_->GetTransactionHistory(from_timestamp_seconds,
      to_timestamp_seconds, callback);
}

scoped_refptr<base::SequencedTaskRunner> LedgerImpl::GetTaskRunner() {
  return task_runner_;
}

}  // namespace bat_ledger

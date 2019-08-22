/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool.h"
#include "bat/ads/issuers_info.h"
#include "bat/ads/notification_info.h"
#include "bat/confirmations/confirmations.h"
#include "bat/ledger/internal/media/media.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/bat_state.h"
#include "bat/ledger/internal/grants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "net/http/http_status_code.h"

using namespace braveledger_grant; //  NOLINT
using namespace braveledger_publisher; //  NOLINT
using namespace braveledger_media; //  NOLINT
using namespace braveledger_bat_state; //  NOLINT
using namespace braveledger_contribution; //  NOLINT
using namespace braveledger_wallet; //  NOLINT
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

bool IsPNG(const std::string& data) {
  return ((data.length() >= 8) &&
          (data.compare(0, 8, "\x89PNG\x0D\x0A\x1A\x0A") == 0));
}

}

namespace bat_ledger {

LedgerImpl::LedgerImpl(ledger::LedgerClient* client) :
    ledger_client_(client),
    bat_grants_(new Grants(this)),
    bat_publisher_(new Publisher(this)),
    bat_media_(new Media(this)),
    bat_state_(new BatState(this)),
    bat_contribution_(new Contribution(this)),
    bat_wallet_(new Wallet(this)),
    initialized_task_scheduler_(false),
    initialized_(false),
    initializing_(false),
    last_tab_active_time_(0),
    last_shown_tab_id_(-1),
    last_pub_load_timer_id_(0u),
    last_grant_check_timer_id_(0u) {
  // Ensure ThreadPoolInstance is initialized before creating the task runner
  // for ios.
  if (!base::ThreadPoolInstance::Get()) {
    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("bat_ledger");

    DCHECK(base::ThreadPoolInstance::Get());
    initialized_task_scheduler_ = true;
  }

  task_runner_ = base::CreateSequencedTaskRunnerWithTraits({
      base::MayBlock(), base::TaskPriority::BEST_EFFORT,
      base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
}

LedgerImpl::~LedgerImpl() {
  if (initialized_task_scheduler_) {
    DCHECK(base::ThreadPoolInstance::Get());
    base::ThreadPoolInstance::Get()->Shutdown();
  }
}

void LedgerImpl::OnWalletInitializedInternal(ledger::Result result,
    ledger::InitializeCallback callback) {
  initializing_ = false;
  callback(result);
  if (result == ledger::Result::LEDGER_OK ||
      result == ledger::Result::WALLET_CREATED) {
    initialized_ = true;
    LoadPublisherList();
    bat_contribution_->SetReconcileTimer();
    RefreshGrant(false);
  } else {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to initialize wallet";
  }
}

void LedgerImpl::Initialize(ledger::InitializeCallback callback) {
  DCHECK(!initializing_);
  initializing_ = true;
  ledger::InitializeCallback on_wallet =
      std::bind(&LedgerImpl::OnWalletInitializedInternal,
                this,
                _1,
                std::move(callback));
  auto on_load = std::bind(&LedgerImpl::OnLedgerStateLoaded,
      this,
      _1,
      _2,
      std::move(on_wallet));
  LoadLedgerState(std::move(on_load));
}

void LedgerImpl::CreateWallet(ledger::CreateWalletCallback callback) {
  if (initializing_) {
    return;
  }

  initializing_ = true;
  auto on_wallet = std::bind(&LedgerImpl::OnWalletInitializedInternal,
      this,
      _1,
      std::move(callback));
  bat_wallet_->CreateWalletIfNecessary(std::move(on_wallet));
}

braveledger_bat_helper::CURRENT_RECONCILE LedgerImpl::GetReconcileById(
    const std::string& viewingId) {
  return bat_state_->GetReconcileById(viewingId);
}

void LedgerImpl::RemoveReconcileById(const std::string& viewingId) {
  bat_state_->RemoveReconcileById(viewingId);
}

void LedgerImpl::OnLoad(ledger::VisitDataPtr visit_data,
                        const uint64_t& current_time) {
  if (visit_data.get()) {
    if (visit_data->domain.empty()) {
      // Skip the same domain name
      return;
    }
    visit_data_iter iter = current_pages_.find(visit_data->tab_id);
    if (iter != current_pages_.end() &&
        iter->second.domain == visit_data->domain) {
      DCHECK(iter == current_pages_.end());
      return;
    }

    if (last_shown_tab_id_ == visit_data->tab_id) {
      last_tab_active_time_ = current_time;
    }
    current_pages_[visit_data->tab_id] = *visit_data;
  }
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

void LedgerImpl::OnSaveVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  // TODO(nejczdovc): handle if needed
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

  auto callback = std::bind(&LedgerImpl::OnSaveVisit,
                            this,
                            _1,
                            _2);

  bat_publisher_->saveVisit(
    iter->second.tld,
    iter->second,
    current_time - last_tab_active_time_,
    0,
    callback);
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

void LedgerImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const std::map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    ledger::VisitDataPtr visit_data) {
  std::string type = bat_media_->GetLinkType(url,
                                                 first_party_url,
                                                 referrer);
  if (type.empty()) {
    // It is not a media supported type
    return;
  }
  bat_media_->ProcessMedia(parts, type, std::move(visit_data));
}

void LedgerImpl::OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      ledger::VisitDataPtr visit_data) {
  std::string type = bat_media_->GetLinkType(url,
                                                 first_party_url,
                                                 referrer);
  if (type.empty()) {
     // It is not a media supported type
    return;
  }

  if (type == TWITCH_MEDIA_TYPE) {
    std::vector<std::map<std::string, std::string>> twitchParts;
    braveledger_media::GetTwitchParts(post_data, &twitchParts);
    for (size_t i = 0; i < twitchParts.size(); i++) {
      bat_media_->ProcessMedia(twitchParts[i], type, std::move(visit_data));
    }
    return;
  }

  if (type == VIMEO_MEDIA_TYPE) {
    std::vector<std::map<std::string, std::string>> parts;
    braveledger_media::GetVimeoParts(post_data, &parts);

    for (auto part = parts.begin(); part != parts.end(); part++) {
      bat_media_->ProcessMedia(*part, type, std::move(visit_data));
    }
    return;
  }
}

void LedgerImpl::LoadLedgerState(ledger::OnLoadCallback callback) {
  ledger_client_->LoadLedgerState(std::move(callback));
}

void LedgerImpl::OnLedgerStateLoaded(ledger::Result result,
                                     const std::string& data,
                                     ledger::InitializeCallback callback) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_state_->LoadState(data)) {
      BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Successfully loaded but failed to parse ledger state.";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed ledger state: " << data;

      callback(ledger::Result::INVALID_LEDGER_STATE);
    } else {
      auto wallet_info = bat_state_->GetWalletInfo();
      SetConfirmationsWalletInfo(wallet_info);
      auto on_pub_load = std::bind(
          &LedgerImpl::OnPublisherStateLoaded,
          this,
          _1,
          _2,
          std::move(callback));
      LoadPublisherState(std::move(on_pub_load));
      bat_contribution_->OnStartUp();
    }
    return;
  }
  if (result != ledger::Result::NO_LEDGER_STATE) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to load ledger state";
    BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
      "Failed ledger state: " <<
      data;
  }
  callback(result);
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

void LedgerImpl::LoadPublisherState(ledger::OnLoadCallback callback) {
  ledger_client_->LoadPublisherState(std::move(callback));
}

void LedgerImpl::OnPublisherStateLoaded(ledger::Result result,
                                        const std::string& data,
                                        ledger::InitializeCallback callback) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_publisher_->loadState(data)) {
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
  if (GetPaymentId().empty() || GetWalletPassphrase().empty()) {
    callback(ledger::Result::CORRUPTED_WALLET);
  } else {
    callback(result);
    OnWalletInitialized(result);
  }
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

void LedgerImpl::LoadPublisherList() {
  ledger_client_->LoadPublisherList(
      std::bind(&LedgerImpl::OnLoadPublisherList, this, _1, _2));
}

void LedgerImpl::OnLoadPublisherList(ledger::Result result,
                                     const std::string& data) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed to load publisher list";
    BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
      "Failed publisher list: " << data;
    RefreshPublishersList(true, true);
    return;
  }

  const auto callback = std::bind(&LedgerImpl::OnParsePublisherList,
      this,
      _1,
      data);
  bat_publisher_->ParsePublisherList(data, callback);
}

void LedgerImpl::OnParsePublisherList(
    const ledger::Result result,
    const std::string& data) {
  if (result == ledger::Result::LEDGER_OK) {
    RefreshPublishersList(false);
    return;
  }

  BLOG(this, ledger::LogLevel::LOG_ERROR) <<
    "Successfully loaded, but failed to parse publish list.";
  BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
    "Failed publisher list: " << data;
  RefreshPublishersList(true);
}

std::string LedgerImpl::GenerateGUID() const {
  return ledger_client_->GenerateGUID();
}

void LedgerImpl::OnWalletInitialized(ledger::Result result) {
  initializing_ = false;

  if (result == ledger::Result::LEDGER_OK ||
      result == ledger::Result::WALLET_CREATED) {
    initialized_ = true;
    LoadPublisherList();
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
    ledger::PublisherInfoPtr info) {
  bat_publisher_->OnPublisherInfoSaved(result, std::move(info));
}

void LedgerImpl::SetPublisherInfo(ledger::PublisherInfoPtr info) {
  if (info) {
    info->verified = bat_publisher_->isVerified(info->id);
  }

  ledger_client_->SavePublisherInfo(
      std::move(info),
      std::bind(&LedgerImpl::OnPublisherInfoSavedInternal,
                this,
                _1,
                _2));
}

void LedgerImpl::SetActivityInfo(ledger::PublisherInfoPtr info) {
  if (info) {
    info->verified = bat_publisher_->isVerified(info->id);
  }

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
                                const uint64_t window_id,
                                const ledger::PublisherInfoCallback callback) {
  uint64_t new_duration = duration;
  if (!bat_publisher_->getPublisherAllowVideos()) {
    new_duration = 0;
  }

  bat_publisher_->saveVisit(publisher_id,
                             visit_data,
                             new_duration,
                             window_id,
                             callback);
}

void LedgerImpl::SetPublisherExclude(
    const std::string& publisher_id,
    const ledger::PUBLISHER_EXCLUDE& exclude,
    ledger::SetPublisherExcludeCallback callback) {
  bat_publisher_->SetPublisherExclude(publisher_id, exclude, callback);
}

void LedgerImpl::RestorePublishers(ledger::RestorePublishersCallback callback) {
  ledger_client_->RestorePublishers(
    std::bind(&LedgerImpl::OnRestorePublishers,
              this,
              _1,
              callback));
}

void LedgerImpl::OnRestorePublishers(
    const ledger::Result result,
    ledger::RestorePublishersCallback callback) {
  bat_publisher_->OnRestorePublishers(result, callback);
}

void LedgerImpl::LoadNicewareList(ledger::GetNicewareListCallback callback) {
  ledger_client_->LoadNicewareList(callback);
}

void LedgerImpl::ModifyPublisherVerified(
    ledger::Result result,
    ledger::PublisherInfoPtr publisher,
    ledger::PublisherInfoCallback callback) {
  if (publisher) {
    publisher->verified = bat_publisher_->isVerified(publisher->id);
  }

  callback(result, std::move(publisher));
}

void LedgerImpl::ModifyPublisherListVerified(
    ledger::PublisherInfoList list,
    uint32_t record,
    ledger::PublisherInfoListCallback callback) {
  ledger::PublisherInfoList new_list;

  for (const auto& publisher : list) {
    auto info = publisher->Clone();
    info->verified = bat_publisher_->isVerified(info->id);
    new_list.push_back(std::move(info));
  }

  callback(std::move(new_list), record);
}

void LedgerImpl::GetPublisherInfo(const std::string& publisher_key,
                                  ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadPublisherInfo(
      publisher_key,
      std::bind(&LedgerImpl::ModifyPublisherVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::GetActivityInfo(ledger::ActivityInfoFilterPtr filter,
                                 ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadActivityInfo(
      std::move(filter),
      std::bind(&LedgerImpl::ModifyPublisherVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::GetPanelPublisherInfo(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadPanelPublisherInfo(
      std::move(filter),
      std::bind(&LedgerImpl::ModifyPublisherVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::GetMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  ledger_client_->LoadMediaPublisherInfo(
      media_key,
      std::bind(&LedgerImpl::ModifyPublisherVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  ledger_client_->GetActivityInfoList(
      start,
      limit,
      std::move(filter),
      std::bind(&LedgerImpl::ModifyPublisherListVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::SetRewardsMainEnabled(bool enabled) {
  bat_state_->SetRewardsMainEnabled(enabled);
}

void LedgerImpl::SetPublisherMinVisitTime(uint64_t duration) {  // In seconds
  bat_publisher_->setPublisherMinVisitTime(duration);
}

void LedgerImpl::SetPublisherMinVisits(unsigned int visits) {
  bat_publisher_->setPublisherMinVisits(visits);
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  bat_publisher_->setPublisherAllowNonVerified(allow);
}

void LedgerImpl::SetPublisherAllowVideos(bool allow) {
  bat_publisher_->setPublisherAllowVideos(allow);
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

ledger::AutoContributePropsPtr LedgerImpl::GetAutoContributeProps() {
  ledger::AutoContributePropsPtr props = ledger::AutoContributeProps::New();
  props->enabled_contribute = GetAutoContribute();
  props->contribution_min_time = GetPublisherMinVisitTime();
  props->contribution_min_visits = GetPublisherMinVisits();
  props->contribution_non_verified = GetPublisherAllowNonVerified();
  props->contribution_videos = GetPublisherAllowVideos();
  props->reconcile_stamp = GetReconcileStamp();
  return props;
}

bool LedgerImpl::GetRewardsMainEnabled() const {
  return bat_state_->GetRewardsMainEnabled();
}

uint64_t LedgerImpl::GetPublisherMinVisitTime() const {
  return bat_publisher_->getPublisherMinVisitTime();
}

unsigned int LedgerImpl::GetPublisherMinVisits() const {
  return bat_publisher_->GetPublisherMinVisits();
}

bool LedgerImpl::GetPublisherAllowNonVerified() const {
  return bat_publisher_->getPublisherAllowNonVerified();
}

bool LedgerImpl::GetPublisherAllowVideos() const {
  return bat_publisher_->getPublisherAllowVideos();
}

double LedgerImpl::GetContributionAmount() const {
  return bat_state_->GetContributionAmount();
}

bool LedgerImpl::GetAutoContribute() const {
  return bat_state_->GetAutoContribute();
}

uint64_t LedgerImpl::GetReconcileStamp() const {
  return bat_state_->GetReconcileStamp();
}

void LedgerImpl::OnReconcileComplete(ledger::Result result,
                                     const std::string& viewing_id,
                                     const std::string& probi,
                                     int32_t category) {
  auto reconcile = GetReconcileById(viewing_id);

  if (category == 0) {
    category = reconcile.category_;
  }

  ledger_client_->OnReconcileComplete(
      result,
      viewing_id,
      static_cast<ledger::REWARDS_CATEGORY>(category),
      probi);
}

void LedgerImpl::OnWalletProperties(
    ledger::Result result,
    const braveledger_bat_helper::WALLET_PROPERTIES_ST& properties) {
  ledger::WalletPropertiesPtr wallet;

  if (result == ledger::Result::LEDGER_OK) {
    wallet = bat_wallet_->WalletPropertiesToWalletInfo(properties);
  }

  ledger_client_->OnWalletProperties(result, std::move(wallet));
}

void LedgerImpl::FetchWalletProperties(
    ledger::OnWalletPropertiesCallback callback) const {
  bat_wallet_->GetWalletProperties(callback);
}

void LedgerImpl::FetchGrants(const std::string& lang,
                             const std::string& payment_id,
                             ledger::FetchGrantsCallback callback) const {
  bat_grants_->FetchGrants(lang, payment_id, callback);
}

void LedgerImpl::OnGrants(ledger::Result result,
                          const braveledger_bat_helper::Grants& grants,
                          ledger::FetchGrantsCallback callback) {
  std::vector<ledger::GrantPtr> ledger_grants;
  for (const braveledger_bat_helper::GRANT& properties : grants) {
    ledger::GrantPtr grant = ledger::Grant::New();
    grant->type = properties.type;
    grant->promotion_id = properties.promotionId;
    ledger_grants.push_back(std::move(grant));
  }

  last_grant_check_timer_id_ = 0;
  RefreshGrant(result != ledger::Result::LEDGER_OK &&
    result != ledger::Result::GRANT_NOT_FOUND);
  callback(result, std::move(ledger_grants));
}

void LedgerImpl::GetGrantCaptcha(
    const std::vector<std::string>& headers,
    ledger::GetGrantCaptchaCallback callback) const {
  bat_grants_->GetGrantCaptcha(headers, std::move(callback));
}

std::string LedgerImpl::GetWalletPassphrase() const {
  return bat_wallet_->GetWalletPassphrase();
}

void LedgerImpl::RecoverWallet(const std::string& passPhrase) const {
  bat_wallet_->RecoverWallet(passPhrase);
}

void LedgerImpl::OnRecoverWallet(
    const ledger::Result result,
    double balance,
    const std::vector<braveledger_bat_helper::GRANT>& grants) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to recover wallet";
  }

  std::vector<ledger::GrantPtr> ledgerGrants;

  for (size_t i = 0; i < grants.size(); i ++) {
    ledger::GrantPtr tempGrant = ledger::Grant::New();

    tempGrant->altcurrency = grants[i].altcurrency;
    tempGrant->probi = grants[i].probi;
    tempGrant->expiry_time = grants[i].expiryTime;
    tempGrant->type = grants[i].type;

    ledgerGrants.push_back(std::move(tempGrant));
  }

  if (result == ledger::Result::LEDGER_OK) {
    bat_publisher_->clearAllBalanceReports();
  }

  ledger_client_->OnRecoverWallet(result,
                                  balance,
                                  std::move(ledgerGrants));
}

void LedgerImpl::SolveGrantCaptcha(
    const std::string& solution,
    const std::string& promotionId) const {
  bat_grants_->SetGrant(solution, promotionId);
}

void LedgerImpl::OnGrantFinish(ledger::Result result,
                               const braveledger_bat_helper::GRANT& grant) {
  ledger::GrantPtr newGrant = ledger::Grant::New();

  newGrant->altcurrency = grant.altcurrency;
  newGrant->probi = grant.probi;
  newGrant->expiry_time = grant.expiryTime;
  newGrant->promotion_id = grant.promotionId;
  newGrant->type = grant.type;

  if (grant.type == "ads") {
    bat_confirmations_->UpdateAdsRewards(true);
  }

  ledger_client_->OnGrantFinish(result, std::move(newGrant));
}

bool LedgerImpl::GetBalanceReport(
    ledger::ACTIVITY_MONTH month,
    int year,
    ledger::BalanceReportInfo* report_info) const {
  return bat_publisher_->getBalanceReport(month, year, report_info);
}

std::map<std::string, ledger::BalanceReportInfoPtr>
LedgerImpl::GetAllBalanceReports() const {
  return bat_publisher_->GetAllBalanceReports();
}

void LedgerImpl::SetBalanceReport(
    ledger::ACTIVITY_MONTH month,
    int year,
    const ledger::BalanceReportInfo& report_info) {
  bat_publisher_->setBalanceReport(month, year, report_info);
}

void LedgerImpl::SaveUnverifiedContribution(
    ledger::PendingContributionList list,
    ledger::SavePendingContributionCallback callback) {
  ledger_client_->SavePendingContribution(std::move(list), callback);
}

void LedgerImpl::OnSaveUnverifiedTip(
    ledger::DoDirectTipCallback callback,
    ledger::Result result) {
  callback(result);
}

void LedgerImpl::DoDirectTip(const std::string& publisher_id,
                             int amount,
                             const std::string& currency,
                             ledger::DoDirectTipCallback callback) {
  if (publisher_id.empty()) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed direct donation due to missing publisher id";

    // TODO(anyone) add error flow
    callback(ledger::Result::NOT_FOUND);
    return;
  }

  bool is_verified = bat_publisher_->isVerified(publisher_id);

  // Save to the pending list if not verified
  if (!is_verified) {
    auto contribution = ledger::PendingContribution::New();
    contribution->publisher_key = publisher_id;
    contribution->amount = amount;
    contribution->category = ledger::REWARDS_CATEGORY::ONE_TIME_TIP;

    ledger::PendingContributionList list;
    list.push_back(std::move(contribution));

    SaveUnverifiedContribution(
        std::move(list),
        std::bind(&LedgerImpl::OnSaveUnverifiedTip,
                  this,
                  std::move(callback),
                  _1));

    return;
  }

  auto direction = braveledger_bat_helper::RECONCILE_DIRECTION(publisher_id,
                                                               amount,
                                                               currency);
  auto direction_list =
      std::vector<braveledger_bat_helper::RECONCILE_DIRECTION> { direction };
  bat_contribution_->InitReconcile(ledger::REWARDS_CATEGORY::ONE_TIME_TIP,
                                   {},
                                   direction_list);
  callback(ledger::Result::LEDGER_OK);
}

void LedgerImpl::DownloadPublisherList(
    ledger::LoadURLCallback callback) {
  std::vector<std::string> headers;
  headers.push_back("Accept-Encoding: gzip");

  // download the list
  std::string url = braveledger_bat_helper::buildURL(
      GET_PUBLISHERS_LIST,
      std::string(), braveledger_bat_helper::SERVER_TYPES::PUBLISHER_DISTRO);
  LoadURL(
      url,
      headers,
      std::string(),
      std::string(),
      ledger::URL_METHOD::GET,
      callback);
}

void LedgerImpl::OnTimer(uint32_t timer_id) {
  if (bat_confirmations_->OnTimer(timer_id))
    return;

  if (timer_id == last_pub_load_timer_id_) {
    last_pub_load_timer_id_ = 0;

    DownloadPublisherList(
        std::bind(&LedgerImpl::LoadPublishersListCallback,
          this, _1, _2, _3));

  } else if (timer_id == last_grant_check_timer_id_) {
    last_grant_check_timer_id_ = 0;
    FetchGrants(std::string(), std::string(),
                [](ledger::Result _, std::vector<ledger::GrantPtr> __){});
  }

  bat_contribution_->OnTimer(timer_id);
}

void LedgerImpl::SaveRecurringTip(
    ledger::ContributionInfoPtr info,
    ledger::SaveRecurringTipCallback callback) {
  ledger_client_->SaveRecurringTip(std::move(info),
                                   callback);
}

void LedgerImpl::GetRecurringTips(
    ledger::PublisherInfoListCallback callback) {
  ledger_client_->GetRecurringTips(
      std::bind(&LedgerImpl::ModifyPublisherListVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::GetOneTimeTips(
    ledger::PublisherInfoListCallback callback) {
  ledger_client_->GetOneTimeTips(
      std::bind(&LedgerImpl::ModifyPublisherListVerified,
                this,
                _1,
                _2,
                callback));
}

void LedgerImpl::LoadPublishersListCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  LogResponse(__func__, response_status_code, "Publisher list", headers);
  if (response_status_code == net::HTTP_OK && !response.empty()) {
    bat_publisher_->RefreshPublishersList(response);
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
        bat_publisher_->getLastPublishersListLoadTimestamp();

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
  bat_publisher_->OnPublishersListSaved(result);
  RefreshPublishersList(retryAfterError);
}

bool LedgerImpl::IsWalletCreated() const {
  return bat_state_->IsWalletCreated();
}

void LedgerImpl::GetPublisherActivityFromUrl(
    uint64_t windowId,
    ledger::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  bat_publisher_->getPublisherActivityFromUrl(
      windowId,
      *visit_data,
      publisher_blob);
}

void LedgerImpl::GetMediaActivityFromUrl(
    uint64_t windowId,
    ledger::VisitDataPtr visit_data,
    const std::string& providerType,
    const std::string& publisher_blob) {
  bat_media_->GetMediaActivityFromUrl(windowId,
                                          std::move(visit_data),
                                          providerType,
                                          publisher_blob);
}

void LedgerImpl::OnPanelPublisherInfo(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t windowId) {
  ledger_client_->OnPanelPublisherInfo(result, std::move(info), windowId);
}

void LedgerImpl::SetBalanceReportItem(ledger::ACTIVITY_MONTH month,
                                      int year,
                                      ledger::ReportType type,
                                      const std::string& probi) {
  bat_publisher_->setBalanceReportItem(month, year, type, probi);
}

void LedgerImpl::FetchFavIcon(const std::string& url,
                              const std::string& favicon_key,
                              ledger::FetchIconCallback callback) {
  ledger_client_->FetchFavIcon(url, favicon_key, callback);
}

void LedgerImpl::GetPublisherBanner(const std::string& publisher_id,
                                    ledger::PublisherBannerCallback callback) {
  bat_publisher_->GetPublisherBanner(publisher_id, callback);
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

void LedgerImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::RemoveRecurringTipCallback callback) {
  ledger_client_->RemoveRecurringTip(
      publisher_key,
      std::bind(&LedgerImpl::OnRemoveRecurringTip,
                this,
                _1,
                callback));
}

void LedgerImpl::OnRemoveRecurringTip(
    const ledger::Result result,
    ledger::RemoveRecurringTipCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Failed to remove recurring tip";

    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(result);
}

ledger::ActivityInfoFilterPtr LedgerImpl::CreateActivityFilter(
    const std::string& publisher_id,
    ledger::ExcludeFilter excluded,
    bool min_duration,
    const uint64_t& currentReconcileStamp,
    bool non_verified,
    bool min_visits) {
  return bat_publisher_->CreateActivityFilter(publisher_id,
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
  std::string stat =
      response_status_code == net::HTTP_OK ? "Success" : "Failure";

  std::string formatted_headers = "";
  for (auto header = headers.begin(); header != headers.end(); ++header) {
    formatted_headers += "> headers " + header->first + ": " + header->second;
    if (header != headers.end()) {
      formatted_headers += "\n";
    }
  }

  std::string response_data = IsPNG(response) ? "<PNG>" : response;
  BLOG(this, ledger::LogLevel::LOG_RESPONSE) << std::endl
    << "[ RESPONSE - " << func_name << " ]" << std::endl
    << "> time: " << std::time(nullptr) << std::endl
    << "> result: " << stat << std::endl
    << "> response: " << response_data << std::endl
    << formatted_headers
    << "[ END RESPONSE ]";
}

void LedgerImpl::UpdateAdsRewards() {
  bat_confirmations_->UpdateAdsRewards(false);
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

  wallet_info.private_key = braveledger_bat_helper::uint8ToHex(secretKey);

  return wallet_info;
}

void LedgerImpl::GetRewardsInternalsInfo(
    ledger::RewardsInternalsInfoCallback callback) {
  ledger::RewardsInternalsInfoPtr info = ledger::RewardsInternalsInfo::New();

  // Retrieve the payment id.
  info->payment_id = bat_state_->GetPaymentId();

  // Retrieve the persona id.
  info->persona_id = bat_state_->GetPersonaId();

  // Retrieve the user id.
  info->user_id = bat_state_->GetUserId();

  // Retrieve the boot stamp.
  info->boot_stamp = bat_state_->GetBootStamp();

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
    ledger::ReconcileInfoPtr reconcile_info = ledger::ReconcileInfo::New();
    reconcile_info->viewing_id = reconcile.second.viewingId_;
    reconcile_info->amount = reconcile.second.amount_;
    reconcile_info->retry_step = reconcile.second.retry_step_;
    reconcile_info->retry_level = reconcile.second.retry_level_;
    info->current_reconciles.insert(
        std::make_pair(reconcile.second.viewingId_, std::move(reconcile_info)));
  }

  callback(std::move(info));
}

void LedgerImpl::StartMonthlyContribution() {
  bat_contribution_->StartMonthlyContribution();
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
    const ledger::PublisherInfoList* list,
    uint32_t record) {
  bat_publisher_->NormalizeContributeWinners(newList, list, record);
}

void LedgerImpl::SetTimer(uint64_t time_offset, uint32_t* timer_id) const {
  ledger_client_->SetTimer(time_offset, timer_id);
}

bool LedgerImpl::AddReconcileStep(
    const std::string& viewing_id,
    ledger::ContributionRetry step,
    int level) {
  BLOG(this, ledger::LogLevel::LOG_DEBUG)
    << "Contribution step "
    << std::to_string(static_cast<int32_t>(step))
    << " for "
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

void LedgerImpl::HasSufficientBalanceToReconcile(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  bat_contribution_->HasSufficientBalance(callback);
}

void LedgerImpl::SaveNormalizedPublisherList(
    ledger::PublisherInfoList list) {
  ledger_client_->SaveNormalizedPublisherList(std::move(list));
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

  if (bat_confirmations_) {
    bat_confirmations_->SetCatalogIssuers(std::move(issuers_info));
  }
}

void LedgerImpl::ConfirmAd(const std::string& info) {
  ads::NotificationInfo notification_info_ads;
  if (notification_info_ads.FromJson(info) != ads::Result::SUCCESS)
    return;

  auto notification_info = std::make_unique<confirmations::NotificationInfo>();
  notification_info->id = notification_info_ads.id;
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

    case ads::ConfirmationType::FLAG: {
      notification_info->type = confirmations::ConfirmationType::FLAG;
      break;
    }

    case ads::ConfirmationType::UPVOTE: {
      notification_info->type = confirmations::ConfirmationType::UPVOTE;
      break;
    }

    case ads::ConfirmationType::DOWNVOTE: {
      notification_info->type = confirmations::ConfirmationType::DOWNVOTE;
      break;
    }
  }

  bat_confirmations_->ConfirmAd(std::move(notification_info));
}

void LedgerImpl::ConfirmAction(
    const std::string& uuid,
    const std::string& creative_set_id,
    const std::string& type) {
  bat_confirmations_->ConfirmAction(uuid,
                                    creative_set_id,
                                    confirmations::ConfirmationType(type));
}

void LedgerImpl::GetTransactionHistory(
    ledger::GetTransactionHistoryCallback callback) {
  bat_confirmations_->GetTransactionHistory(callback);
}

void LedgerImpl::RefreshPublisher(
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  DownloadPublisherList(
      std::bind(&LedgerImpl::OnRefreshPublisher,
                this,
                _1,
                _2,
                _3,
                publisher_key,
                callback));
}

void LedgerImpl::OnRefreshPublisher(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  LoadPublishersListCallback(response_status_code, response, headers);
  bat_publisher_->RefreshPublisherVerifiedStatus(
      publisher_key,
      callback);
}

scoped_refptr<base::SequencedTaskRunner> LedgerImpl::GetTaskRunner() {
  return task_runner_;
}

void LedgerImpl::SaveMediaInfo(const std::string& type,
                               const std::map<std::string, std::string>& data,
                               ledger::PublisherInfoCallback callback) {
  bat_media_->SaveMediaInfo(type, data, callback);
}

void LedgerImpl::SetInlineTipSetting(const std::string& key, bool enabled) {
  bat_state_->SetInlineTipSetting(key, enabled);
}

bool LedgerImpl::GetInlineTipSetting(const std::string& key) {
  return bat_state_->GetInlineTipSetting(key);
}

std::string LedgerImpl::GetShareURL(
    const std::string& type,
    const std::map<std::string, std::string>& args) {
  return bat_media_->GetShareURL(type, args);
}

void LedgerImpl::OnGetPendingContributions(
    const ledger::PendingContributionInfoList& list,
    ledger::PendingContributionInfoListCallback callback) {
  ledger::PendingContributionInfoList new_list;
  for (const auto& item : list) {
    auto new_item = item->Clone();
    new_item->expiration_date =
        new_item->added_date +
        braveledger_ledger::_pending_contribution_expiration;

    new_list.push_back(std::move(new_item));
  }

  callback(std::move(new_list));
}

void LedgerImpl::GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) {
  ledger_client_->GetPendingContributions(
      std::bind(&LedgerImpl::OnGetPendingContributions,
                this,
                _1,
                callback));
}

void LedgerImpl::RemovePendingContribution(
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date,
    ledger::RemovePendingContributionCallback callback) {
  ledger_client_->RemovePendingContribution(publisher_key,
                                            viewing_id,
                                            added_date,
                                            callback);
}

void LedgerImpl::RemoveAllPendingContributions(
    ledger::RemovePendingContributionCallback callback) {
  ledger_client_->RemoveAllPendingContributions(callback);
}

void LedgerImpl::GetPendingContributionsTotal(
    ledger::PendingContributionsTotalCallback callback) {
  ledger_client_->GetPendingContributionsTotal(callback);
}

void LedgerImpl::ContributeUnverifiedPublishers() {
  bat_contribution_->ContributeUnverifiedPublishers();
}

bool LedgerImpl::IsPublisherVerified(const std::string& publisher_key) {
  return bat_publisher_->isVerified(publisher_key);
}

void LedgerImpl::OnContributeUnverifiedPublishers(
    ledger::Result result,
    const std::string& publisher_key,
    const std::string& publisher_name) {
  ledger_client_->OnContributeUnverifiedPublishers(result,
                                                   publisher_key,
                                                   publisher_name);
}

void LedgerImpl::SavePublisherProcessed(const std::string& publisher_key) {
  bat_publisher_->SavePublisherProcessed(publisher_key);
}

bool LedgerImpl::WasPublisherAlreadyProcessed(
    const std::string& publisher_key) const {
  return bat_publisher_->WasPublisherAlreadyProcessed(publisher_key);
}

void LedgerImpl::FetchBalance(ledger::FetchBalanceCallback callback) {
  bat_wallet_->FetchBalance(callback);
}

void LedgerImpl::GetExternalWallets(
    ledger::GetExternalWalletsCallback callback) {
  ledger_client_->GetExternalWallets(callback);
}

std::string LedgerImpl::GetPublisherAddress(
    const std::string& publisher_key) const {
  return bat_publisher_->GetPublisherAddress(publisher_key);
}

std::string LedgerImpl::GetCardIdAddress() const {
  return bat_state_->GetCardIdAddress();
}

void LedgerImpl::GetExternalWallet(const std::string& wallet_type,
                                   ledger::ExternalWalletCallback callback) {
  bat_wallet_->GetExternalWallet(wallet_type, callback);
}

void LedgerImpl::SaveExternalWallet(const std::string& wallet_type,
                                    ledger::ExternalWalletPtr wallet) {
  ledger_client_->SaveExternalWallet(wallet_type, std::move(wallet));
}

void LedgerImpl::ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback) {
  bat_wallet_->ExternalWalletAuthorization(
      wallet_type,
      args,
      callback);
}

void LedgerImpl::DisconnectWallet(
      const std::string& wallet_type,
      ledger::DisconnectWalletCallback callback) {
  bat_wallet_->DisconnectWallet(wallet_type, callback);
}

void LedgerImpl::TransferAnonToExternalWallet(
      const std::string& new_address,
      const bool allow_zero_balance,
      ledger::TransferAnonToExternalWalletCallback callback) {
  bat_wallet_->TransferAnonToExternalWallet(
    new_address,
    allow_zero_balance,
    callback);
}

void LedgerImpl::ShowNotification(
      const std::string& type,
      ledger::ShowNotificationCallback callback,
      const std::vector<std::string>& args) {
  ledger_client_->ShowNotification(type, args, callback);
}

void LedgerImpl::DeleteActivityInfo(
      const std::string& publisher_key,
      ledger::DeleteActivityInfoCallback callback) {
  ledger_client_->DeleteActivityInfo(publisher_key, callback);
}

void LedgerImpl::ClearAndInsertServerPublisherList(
      ledger::ServerPublisherInfoList list,
      ledger::ClearAndInsertServerPublisherListCallback callback) {
  ledger_client_->ClearAndInsertServerPublisherList(std::move(list), callback);
}

}  // namespace bat_ledger

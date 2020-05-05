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
#include "base/task/thread_pool/thread_pool_instance.h"
#include "bat/ads/issuers_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/confirmations/confirmations.h"
#include "bat/ledger/internal/media/media.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/bat_state.h"
#include "bat/ledger/internal/promotion/promotion.h"
#include "bat/ledger/internal/report/report.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/static_values.h"
#include "net/http/http_status_code.h"

using namespace braveledger_promotion; //  NOLINT
using namespace braveledger_publisher; //  NOLINT
using namespace braveledger_media; //  NOLINT
using namespace braveledger_bat_state; //  NOLINT
using namespace braveledger_contribution; //  NOLINT
using namespace braveledger_wallet; //  NOLINT
using namespace braveledger_database; //  NOLINT
using namespace braveledger_report; //  NOLINT
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
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
    bat_promotion_(new Promotion(this)),
    bat_publisher_(new Publisher(this)),
    bat_media_(new Media(this)),
    bat_state_(new BatState(this)),
    bat_contribution_(new Contribution(this)),
    bat_wallet_(new Wallet(this)),
    bat_database_(new Database(this)),
    bat_report_(new Report(this)),
    initialized_task_scheduler_(false),
    initialized_(false),
    initializing_(false),
    last_tab_active_time_(0),
    last_shown_tab_id_(-1),
    last_pub_load_timer_id_(0u) {
  // Ensure ThreadPoolInstance is initialized before creating the task runner
  // for ios.
  if (!base::ThreadPoolInstance::Get()) {
    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("bat_ledger");

    DCHECK(base::ThreadPoolInstance::Get());
    initialized_task_scheduler_ = true;
  }

  task_runner_ = base::CreateSequencedTaskRunner(
      {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
}

LedgerImpl::~LedgerImpl() {
  if (initialized_task_scheduler_) {
    DCHECK(base::ThreadPoolInstance::Get());
    base::ThreadPoolInstance::Get()->Shutdown();
  }
}

void LedgerImpl::OnWalletInitializedInternal(
    ledger::Result result,
    ledger::ResultCallback callback) {
  initializing_ = false;
  callback(result);
  if (result == ledger::Result::LEDGER_OK ||
      result == ledger::Result::WALLET_CREATED) {
    initialized_ = true;
    bat_publisher_->SetPublisherServerListTimer(GetRewardsMainEnabled());
    bat_contribution_->SetReconcileTimer();
    bat_promotion_->Refresh(false);
    bat_contribution_->Initialize();
    bat_promotion_->Initialize();

    // Set wallet info for Confirmations when launching the browser or creating
    // a wallet for the first time
    auto wallet_info = bat_state_->GetWalletInfo();
    SetConfirmationsWalletInfo(wallet_info);
  } else {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to initialize wallet";
  }
}

void LedgerImpl::Initialize(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  DCHECK(!initializing_);
  if (initializing_) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Already initializing ledger";
    return;
  }

  initializing_ = true;

  InitializeConfirmations(execute_create_script, callback);
}

void LedgerImpl::InitializeConfirmations(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  confirmations::_environment = ledger::_environment;
  confirmations::_is_debug = ledger::is_debug;

  bat_confirmations_.reset(
      confirmations::Confirmations::CreateInstance(ledger_client_));

  auto initialized_callback = std::bind(&LedgerImpl::OnConfirmationsInitialized,
      this,
      _1,
      execute_create_script,
      callback);
  bat_confirmations_->Initialize(initialized_callback);
}

void LedgerImpl::OnConfirmationsInitialized(
    const bool success,
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  if (!success) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Failed to initialize confirmations";
  }

  ledger::ResultCallback finish_callback =
      std::bind(&LedgerImpl::OnWalletInitializedInternal,
          this,
          _1,
          std::move(callback));

  auto database_callback = std::bind(&LedgerImpl::OnDatabaseInitialized,
      this,
      _1,
      finish_callback);
  bat_database_->Initialize(execute_create_script, database_callback);
}

void LedgerImpl::CreateWallet(ledger::ResultCallback callback) {
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

ledger::CurrentReconcileProperties LedgerImpl::GetReconcileById(
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

void LedgerImpl::OnHide(uint32_t tab_id, const uint64_t& current_time) {
  if (!GetRewardsMainEnabled() || !GetAutoContribute()) {
    return;
  }

  if (tab_id != last_shown_tab_id_ || last_tab_active_time_ == 0) {
    return;
  }

  visit_data_iter iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end()) {
    return;
  }

  const std::string type = bat_media_->GetLinkType(iter->second.tld, "", "");
  const auto duration = current_time - last_tab_active_time_;
  last_tab_active_time_ = 0;

  if (type == GITHUB_MEDIA_TYPE) {
      std::map<std::string, std::string> parts;
      parts["duration"] = std::to_string(duration);
      bat_media_->ProcessMedia(parts, type, iter->second.Clone());
    return;
  }

  bat_publisher_->SaveVisit(
      iter->second.tld,
      iter->second,
      duration,
      0,
      [](ledger::Result, ledger::PublisherInfoPtr){});
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

void LedgerImpl::OnLedgerStateLoaded(
    ledger::Result result,
    const std::string& data,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::LEDGER_OK) {
    if (!bat_state_->LoadState(data)) {
      BLOG(this, ledger::LogLevel::LOG_ERROR) <<
        "Successfully loaded but failed to parse ledger state.";
      BLOG(this, ledger::LogLevel::LOG_DEBUG) <<
        "Failed ledger state: " << data;

      callback(ledger::Result::INVALID_LEDGER_STATE);
    } else {
      auto wallet_info = bat_state_->GetWalletInfo();
      auto on_pub_load = std::bind(
          &LedgerImpl::OnPublisherStateLoaded,
          this,
          _1,
          _2,
          std::move(callback));
      LoadPublisherState(std::move(on_pub_load));
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
    const ledger::WalletInfoProperties& wallet_info_properties) {
  if (wallet_info_properties.key_info_seed.size() != SEED_LENGTH) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to initialize "
        "confirmations due to invalid wallet";
    return;
  }

  const std::vector<uint8_t> seed =
      braveledger_bat_helper::getHKDF(wallet_info_properties.key_info_seed);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> secret_key;

  if (!braveledger_bat_helper::getPublicKeyFromSeed(seed, &public_key,
      &secret_key)) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to initialize "
        "confirmations due to invalid wallet";
    return;
  }

  confirmations::WalletInfo wallet_info;
  wallet_info.payment_id = wallet_info_properties.payment_id;
  wallet_info.private_key = braveledger_bat_helper::uint8ToHex(secret_key);

  if (!wallet_info.IsValid()) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to initialize "
        "confirmations due to invalid wallet";
    return;
  }

  bat_confirmations_->SetWalletInfo(
      std::make_unique<confirmations::WalletInfo>(wallet_info));
}

void LedgerImpl::LoadPublisherState(ledger::OnLoadCallback callback) {
  ledger_client_->LoadPublisherState(std::move(callback));
}

void LedgerImpl::OnPublisherStateLoaded(
    ledger::Result result,
    const std::string& data,
    ledger::ResultCallback callback) {
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
    return;
  }

  callback(result);
}

void LedgerImpl::OnDatabaseInitialized(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) <<
      "Database could not be initialized. Error: " << result;
    callback(result);
    return;
  }

  auto on_load = std::bind(&LedgerImpl::OnLedgerStateLoaded,
      this,
      _1,
      _2,
      std::move(callback));
  LoadLedgerState(on_load);
}

void LedgerImpl::SaveLedgerState(
    const std::string& data,
    ledger::ResultCallback callback) {
  ledger_client_->SaveLedgerState(data, callback);
}

void LedgerImpl::SavePublisherState(
    const std::string& data,
    ledger::ResultCallback callback) {
  ledger_client_->SavePublisherState(data, callback);
}

void LedgerImpl::LogRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const ledger::UrlMethod method) {
  std::string formatted_headers = "";
  for (const auto & header : headers) {
    formatted_headers += "> header: " + header + "\n";
  }

  BLOG(this, ledger::LogLevel::LOG_REQUEST) << std::endl
      << "[ REQUEST ]" << std::endl
      << "> url: " << url << std::endl
      << "> method: " << method << std::endl
      << "> content: " << content << std::endl
      << "> contentType: " << content_type << std::endl
      << formatted_headers
      << "[ END REQUEST ]";
}

void LedgerImpl::LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const ledger::UrlMethod method,
    ledger::LoadURLCallback callback) {
  LogRequest(url, headers, content, content_type, method);

  ledger_client_->LoadURL(
      url,
      headers,
      content,
      content_type,
      method,
      callback);
}

std::string LedgerImpl::URIEncode(const std::string& value) {
  return ledger_client_->URIEncode(value);
}

void LedgerImpl::SavePublisherInfo(
    ledger::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SavePublisherInfo(std::move(info), callback);
}

void LedgerImpl::SaveActivityInfo(
    ledger::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveActivityInfo(std::move(info), callback);
}

void LedgerImpl::SaveMediaPublisherInfo(
    const std::string& media_key,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  bat_database_->SaveMediaPublisherInfo(media_key, publisher_key, callback);
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

  bat_publisher_->SaveVisit(publisher_id,
                             visit_data,
                             new_duration,
                             window_id,
                             callback);
}

void LedgerImpl::SetPublisherExclude(
    const std::string& publisher_id,
    const ledger::PublisherExclude& exclude,
    ledger::ResultCallback callback) {
  bat_publisher_->SetPublisherExclude(publisher_id, exclude, callback);
}

void LedgerImpl::RestorePublishers(ledger::ResultCallback callback) {
  bat_database_->RestorePublishers(
    std::bind(&LedgerImpl::OnRestorePublishers,
              this,
              _1,
              callback));
}

void LedgerImpl::OnRestorePublishers(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  bat_publisher_->OnRestorePublishers(result, callback);
}

void LedgerImpl::LoadNicewareList(ledger::GetNicewareListCallback callback) {
  ledger_client_->LoadNicewareList(callback);
}

void LedgerImpl::GetPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  bat_database_->GetPublisherInfo(publisher_key, callback);
}

void LedgerImpl::OnGetActivityInfo(
    ledger::PublisherInfoList list,
    ledger::PublisherInfoCallback callback,
    const std::string& publisher_key) {
  if (list.empty()) {
    GetPublisherInfo(publisher_key, callback);
    return;
  }

  if (list.size() > 1) {
    callback(ledger::Result::TOO_MANY_RESULTS, nullptr);
    return;
  }

  callback(ledger::Result::LEDGER_OK, std::move(list[0]));
}

void LedgerImpl::GetActivityInfo(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  auto list_callback = std::bind(&LedgerImpl::OnGetActivityInfo,
      this,
      _1,
      callback,
      filter->id);

  bat_database_->GetActivityInfoList(
      0,
      2,
      std::move(filter),
      list_callback);
}

void LedgerImpl::GetPanelPublisherInfo(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  bat_database_->GetPanelPublisherInfo(std::move(filter), callback);
}

void LedgerImpl::GetMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  bat_database_->GetMediaPublisherInfo(media_key, callback);
}

void LedgerImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  bat_database_->GetActivityInfoList(
      start,
      limit,
      std::move(filter),
      callback);
}

void LedgerImpl::GetExcludedList(ledger::PublisherInfoListCallback callback) {
  bat_database_->GetExcludedList(callback);
}

void LedgerImpl::SetRewardsMainEnabled(bool enabled) {
  bat_state_->SetRewardsMainEnabled(enabled);
  bat_publisher_->SetPublisherServerListTimer(enabled);
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

void LedgerImpl::ReconcileComplete(
    const ledger::Result result,
    const double amount,
    const std::string& viewing_id,
    const ledger::RewardsType type,
    const bool delete_reconcile) {
  const auto reconcile = GetReconcileById(viewing_id);

  if (result == ledger::Result::LEDGER_OK) {
    bat_contribution_->ReconcileSuccess(
      viewing_id,
      amount,
      delete_reconcile);
  }

  ledger_client_->OnReconcileComplete(
      result,
      viewing_id,
      amount,
      type);
}

void LedgerImpl::ContributionCompleted(
    const ledger::Result result,
    const double amount,
    const std::string& contribution_id,
    const ledger::RewardsType type) {
  bat_contribution_->ContributionCompleted(
      contribution_id,
      type,
      amount,
      result);

  // TODO(https://github.com/brave/brave-browser/issues/7717)
  // rename to ContributionCompleted
  ledger_client_->OnReconcileComplete(
      result,
      contribution_id,
      amount,
      type);
}

void LedgerImpl::OnWalletProperties(
    ledger::Result result,
    const ledger::WalletProperties& properties) {
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

void LedgerImpl::ClaimPromotion(
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) const {
  bat_promotion_->Claim(payload, std::move(callback));
}

void LedgerImpl::AttestPromotion(
    const std::string& promotion_id,
    const std::string& solution,
    ledger::AttestPromotionCallback callback) const {
  bat_promotion_->Attest(promotion_id, solution, callback);
}

std::string LedgerImpl::GetWalletPassphrase() const {
  return bat_wallet_->GetWalletPassphrase();
}

void LedgerImpl::RecoverWallet(
    const std::string& pass_phrase,
    ledger::RecoverWalletCallback callback) {
  auto on_recover = std::bind(&LedgerImpl::OnRecoverWallet,
      this,
      _1,
      _2,
      std::move(callback));
  bat_wallet_->RecoverWallet(pass_phrase, std::move(on_recover));
}

void LedgerImpl::OnRecoverWallet(
    const ledger::Result result,
    const double balance,
    ledger::RecoverWalletCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(this, ledger::LogLevel::LOG_ERROR) << "Failed to recover wallet";
  }

  if (result == ledger::Result::LEDGER_OK) {
    bat_publisher_->clearAllBalanceReports();
  }

  callback(result, balance);
}

void LedgerImpl::GetBalanceReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetBalanceReportCallback callback) const {
  bat_publisher_->GetBalanceReport(month, year, callback);
}

std::map<std::string, ledger::BalanceReportInfoPtr>
LedgerImpl::GetAllBalanceReports() const {
  return bat_publisher_->GetAllBalanceReports();
}

void LedgerImpl::SavePendingContribution(
    ledger::PendingContributionList list,
    ledger::ResultCallback callback) {
  bat_database_->SavePendingContribution(std::move(list), callback);
}

void LedgerImpl::PendingContributionSaved(const ledger::Result result) {
  ledger_client_->PendingContributionSaved(result);
}

void LedgerImpl::OneTimeTip(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  bat_contribution_->OneTimeTip(publisher_key, amount, callback);
}

void LedgerImpl::OnTimer(uint32_t timer_id) {
  bat_contribution_->OnTimer(timer_id);
  bat_publisher_->OnTimer(timer_id);
  bat_promotion_->OnTimer(timer_id);
}

void LedgerImpl::SaveRecurringTip(
    ledger::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveRecurringTip(
      std::move(info),
      callback);
}

void LedgerImpl::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  bat_database_->GetRecurringTips(callback);
}

void LedgerImpl::GetOneTimeTips(ledger::PublisherInfoListCallback callback) {
  bat_database_->GetOneTimeTips(
      braveledger_time_util::GetCurrentMonth(),
      braveledger_time_util::GetCurrentYear(),
      callback);
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

void LedgerImpl::SetBalanceReportItem(
    const ledger::ActivityMonth month,
    const int year,
    const ledger::ReportType type,
    const double amount) {
  bat_publisher_->SetBalanceReportItem(month, year, type, amount);
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

void LedgerImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  bat_database_->RemoveRecurringTip(publisher_key, callback);
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
  const std::string result =
      response_status_code >= 200 && response_status_code < 300
          ? "Success" : "Failure";

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
    << "> result: " << result << std::endl
    << "> http code: " << response_status_code << std::endl
    << "> response: " << response_data << std::endl
    << formatted_headers
    << "[ END RESPONSE ]";
}

void LedgerImpl::UpdateAdsRewards() {
  bat_confirmations_->UpdateAdsRewards(false);
}

void LedgerImpl::ResetReconcileStamp() {
  bat_state_->ResetReconcileStamp();
  ledger_client_->ReconcileStampReset();
}

bool LedgerImpl::UpdateReconcile(
    const ledger::CurrentReconcileProperties& reconcile) {
  return bat_state_->UpdateReconcile(reconcile);
}

void LedgerImpl::AddReconcile(
      const std::string& viewing_id,
      const ledger::CurrentReconcileProperties& reconcile) {
  bat_state_->AddReconcile(viewing_id, reconcile);
}

const std::string& LedgerImpl::GetPaymentId() const {
  return bat_state_->GetPaymentId();
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

const ledger::WalletInfoProperties& LedgerImpl::GetWalletInfo() const {
  return bat_state_->GetWalletInfo();
}

void LedgerImpl::SetWalletInfo(
    const ledger::WalletInfoProperties& info) {
  bat_state_->SetWalletInfo(info);

  if (!initializing_) {
    // Only update wallet info for Confirmations if |SetWalletInfo| was not
    // called when launching the browser or creating a wallet for the first time
    // (i.e. recovering a wallet), as these scenarios are covered in
    // |OnWalletInitializedInternal|
    SetConfirmationsWalletInfo(info);
  }
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
  const ledger::WalletInfoProperties wallet_info = bat_state_->GetWalletInfo();
  if (wallet_info.key_info_seed.size() != SEED_LENGTH) {
    info->is_key_info_seed_valid = false;
  } else {
    std::vector<uint8_t> secret_key =
        braveledger_bat_helper::getHKDF(wallet_info.key_info_seed);
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> new_secret_key;
    info->is_key_info_seed_valid = braveledger_bat_helper::getPublicKeyFromSeed(
        secret_key, &public_key, &new_secret_key);
  }

  // Retrieve the current reconciles.
  const ledger::CurrentReconciles current_reconciles = GetCurrentReconciles();
  for (const auto& reconcile : current_reconciles) {
    ledger::ReconcileInfoPtr reconcile_info = ledger::ReconcileInfo::New();
    reconcile_info->viewing_id = reconcile.second.viewing_id;
    reconcile_info->amount = reconcile.second.amount;
    reconcile_info->retry_step = reconcile.second.retry_step;
    reconcile_info->retry_level = reconcile.second.retry_level;
    info->current_reconciles.insert(
        std::make_pair(reconcile.second.viewing_id, std::move(reconcile_info)));
  }

  callback(std::move(info));
}

void LedgerImpl::StartMonthlyContribution() {
  bat_contribution_->StartMonthlyContribution();
}

const ledger::WalletProperties& LedgerImpl::GetWalletProperties() const {
  return bat_state_->GetWalletProperties();
}

void LedgerImpl::SetWalletProperties(
    ledger::WalletProperties* properties) {
  bat_state_->SetWalletProperties(properties);
}

unsigned int LedgerImpl::GetDays() const {
  return bat_state_->GetDays();
}

void LedgerImpl::SetDays(unsigned int days) {
  bat_state_->SetDays(days);
}

const ledger::Transactions& LedgerImpl::GetTransactions() const {
  return bat_state_->GetTransactions();
}

void LedgerImpl::SetTransactions(
    const ledger::Transactions& transactions) {
  bat_state_->SetTransactions(transactions);
}

const ledger::Ballots& LedgerImpl::GetBallots() const {
  return bat_state_->GetBallots();
}

void LedgerImpl::SetBallots(const ledger::Ballots& ballots) {
  bat_state_->SetBallots(ballots);
}

const ledger::PublisherVotes& LedgerImpl::GetPublisherVotes() const {
  return bat_state_->GetPublisherVotes();
}

void LedgerImpl::SetPublisherVotes(
    const ledger::PublisherVotes& publisher_votes) {
  bat_state_->SetPublisherVotes(publisher_votes);
}

const std::string& LedgerImpl::GetCurrency() const {
  return bat_state_->GetCurrency();
}

void LedgerImpl::SetCurrency(const std::string& currency) {
  bat_state_->SetCurrency(currency);
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
    ledger::ContributionInfoPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveContributionInfo(std::move(info), callback);
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

const ledger::CurrentReconciles& LedgerImpl::GetCurrentReconciles() const {
  return bat_state_->GetCurrentReconciles();
}

double LedgerImpl::GetDefaultContributionAmount() {
  return bat_state_->GetDefaultContributionAmount();
}

void LedgerImpl::HasSufficientBalanceToReconcile(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  bat_contribution_->HasSufficientBalance(callback);
}

void LedgerImpl::SaveNormalizedPublisherList(ledger::PublisherInfoList list) {
  ledger::PublisherInfoList save_list;
  for (auto& item : list) {
    save_list.push_back(item.Clone());
  }

  bat_database_->SaveActivityInfoList(
      std::move(save_list),
      [](const ledger::Result){});
  ledger_client_->PublisherListNormalized(std::move(list));
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

void LedgerImpl::ConfirmAd(
    const std::string& json,
    const std::string& confirmation_type) {
  ads::AdInfo ad_info;
  if (ad_info.FromJson(json) != ads::Result::SUCCESS) {
    return;
  }

  confirmations::AdInfo confirmations_ad_info;
  confirmations_ad_info.creative_instance_id = ad_info.creative_instance_id;
  confirmations_ad_info.creative_set_id = ad_info.creative_set_id;
  confirmations_ad_info.category = ad_info.category;
  confirmations_ad_info.target_url = ad_info.target_url;

  bat_confirmations_->ConfirmAd(confirmations_ad_info,
      confirmations::ConfirmationType(confirmation_type));
}

void LedgerImpl::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const std::string& confirmation_type) {
  bat_confirmations_->ConfirmAction(creative_instance_id, creative_set_id,
      confirmations::ConfirmationType(confirmation_type));
}

void LedgerImpl::GetTransactionHistory(
    ledger::GetTransactionHistoryCallback callback) {
  bat_confirmations_->GetTransactionHistory(callback);
}

void LedgerImpl::RefreshPublisher(
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  bat_publisher_->RefreshPublisher(publisher_key, callback);
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

void LedgerImpl::GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) {
  bat_database_->GetPendingContributions(callback);
}

void LedgerImpl::RemovePendingContribution(
    const uint64_t id,
    ledger::ResultCallback callback) {
  bat_database_->RemovePendingContribution(id, callback);
}

void LedgerImpl::RemoveAllPendingContributions(
    ledger::ResultCallback callback) {
  bat_database_->RemoveAllPendingContributions(callback);
}

void LedgerImpl::GetPendingContributionsTotal(
    ledger::PendingContributionsTotalCallback callback) {
  bat_database_->GetPendingContributionsTotal(callback);
}

void LedgerImpl::ContributeUnverifiedPublishers() {
  bat_contribution_->ContributeUnverifiedPublishers();
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
      ledger::ResultCallback callback) {
  bat_wallet_->DisconnectWallet(wallet_type, callback);
}

void LedgerImpl::TransferAnonToExternalWallet(
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback,
    const bool allow_zero_balance) {
  bat_wallet_->TransferAnonToExternalWallet(
    std::move(wallet),
    allow_zero_balance,
    callback);
}

void LedgerImpl::ShowNotification(
      const std::string& type,
      ledger::ResultCallback callback,
      const std::vector<std::string>& args) {
  ledger_client_->ShowNotification(type, args, callback);
}

void LedgerImpl::DeleteActivityInfo(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  bat_database_->DeleteActivityInfo(publisher_key, callback);
}

void LedgerImpl::ClearServerPublisherList(ledger::ResultCallback callback) {
  bat_database_->ClearServerPublisherList(callback);
}

void LedgerImpl::InsertServerPublisherList(
    const std::vector<ledger::ServerPublisherPartial>& list,
    ledger::ResultCallback callback) {
  bat_database_->InsertServerPublisherList(list, callback);
}

void LedgerImpl::InsertPublisherBannerList(
    const std::vector<ledger::PublisherBanner>& list,
    ledger::ResultCallback callback) {
  bat_database_->InsertPublisherBannerList(list, callback);
}

void LedgerImpl::GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  bat_database_->GetServerPublisherInfo(publisher_key, callback);
}

bool LedgerImpl::IsPublisherConnectedOrVerified(
    const ledger::PublisherStatus status) {
  return bat_publisher_->IsConnectedOrVerified(status);
}

void LedgerImpl::SetBooleanState(const std::string& name, bool value) {
  ledger_client_->SetBooleanState(name, value);
}

bool LedgerImpl::GetBooleanState(const std::string& name) const {
  return ledger_client_->GetBooleanState(name);
}

void LedgerImpl::SetIntegerState(const std::string& name, int value) {
  ledger_client_->SetIntegerState(name, value);
}

int LedgerImpl::GetIntegerState(const std::string& name) const {
  return ledger_client_->GetIntegerState(name);
}

void LedgerImpl::SetDoubleState(const std::string& name, double value) {
  ledger_client_->SetDoubleState(name, value);
}

double LedgerImpl::GetDoubleState(const std::string& name) const {
  return ledger_client_->GetDoubleState(name);
}

void LedgerImpl::SetStringState(
    const std::string& name,
    const std::string& value) {
  ledger_client_->SetStringState(name, value);
}

std::string LedgerImpl::GetStringState(const std::string& name) const {
  return ledger_client_->GetStringState(name);
}

void LedgerImpl::SetInt64State(const std::string& name, int64_t value) {
  ledger_client_->SetInt64State(name, value);
}

int64_t LedgerImpl::GetInt64State(const std::string& name) const {
  return ledger_client_->GetInt64State(name);
}

void LedgerImpl::SetUint64State(const std::string& name, uint64_t value) {
  ledger_client_->SetUint64State(name, value);
}

uint64_t LedgerImpl::GetUint64State(const std::string& name) const {
  return ledger_client_->GetUint64State(name);
}

void LedgerImpl::ClearState(const std::string& name) {
  ledger_client_->ClearState(name);
}

bool LedgerImpl::GetBooleanOption(const std::string& name) const {
  return ledger_client_->GetBooleanOption(name);
}

int LedgerImpl::GetIntegerOption(const std::string& name) const {
  return ledger_client_->GetIntegerOption(name);
}

double LedgerImpl::GetDoubleOption(const std::string& name) const {
  return ledger_client_->GetDoubleOption(name);
}

std::string LedgerImpl::GetStringOption(const std::string& name) const {
  return ledger_client_->GetStringOption(name);
}

int64_t LedgerImpl::GetInt64Option(const std::string& name) const {
  return ledger_client_->GetInt64Option(name);
}

uint64_t LedgerImpl::GetUint64Option(const std::string& name) const {
  return ledger_client_->GetUint64Option(name);
}

void LedgerImpl::SetTransferFee(
    const std::string& wallet_type,
    ledger::TransferFeePtr transfer_fee) {
  ledger_client_->SetTransferFee(wallet_type, std::move(transfer_fee));
}

ledger::TransferFeeList LedgerImpl::GetTransferFees(
    const std::string& wallet_type) const {
  return ledger_client_->GetTransferFees(wallet_type);
}

void LedgerImpl::RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) {
  ledger_client_->RemoveTransferFee(wallet_type, id);
}

void LedgerImpl::SaveContributionQueue(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveContributionQueue(std::move(info), callback);
}

void LedgerImpl::DeleteContributionQueue(
    const uint64_t id,
    ledger::ResultCallback callback) {
  bat_database_->DeleteContributionQueue(id, callback);
}

void LedgerImpl::GetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback) {
  bat_database_->GetFirstContributionQueue(callback);
}

void LedgerImpl::FetchPromotions(
    ledger::FetchPromotionCallback callback) const {
  bat_promotion_->Fetch(callback);
}

void LedgerImpl::SavePromotion(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SavePromotion(std::move(info), callback);
}

void LedgerImpl::GetPromotion(
    const std::string& id,
    ledger::GetPromotionCallback callback) {
  bat_database_->GetPromotion(id, callback);
}

void LedgerImpl::GetAllPromotions(
    ledger::GetAllPromotionsCallback callback) {
  bat_database_->GetAllPromotions(callback);
}

void LedgerImpl::DeletePromotionList(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  bat_database_->DeletePromotionList(ids, callback);
}

void LedgerImpl::SaveUnblindedTokenList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  bat_database_->SaveUnblindedTokenList(std::move(list), callback);
}

void LedgerImpl::GetAllUnblindedTokens(
    ledger::GetUnblindedTokenListCallback callback) {
  bat_database_->GetAllUnblindedTokens(callback);
}

void LedgerImpl::DeleteUnblindedTokens(
    const std::vector<std::string>& id_list,
    ledger::ResultCallback callback) {
  bat_database_->DeleteUnblindedTokens(id_list, callback);
}

void LedgerImpl::GetUnblindedTokensByTriggerIds(
    const std::vector<std::string>& trigger_ids,
    ledger::GetUnblindedTokenListCallback callback) {
  bat_database_->GetUnblindedTokensByTriggerIds(trigger_ids, callback);
}

ledger::ClientInfoPtr LedgerImpl::GetClientInfo() {
  return ledger_client_->GetClientInfo();
}

void LedgerImpl::UnblindedTokensReady() {
  return ledger_client_->UnblindedTokensReady();
}

void LedgerImpl::GetAnonWalletStatus(ledger::ResultCallback callback) {
  bat_wallet_->GetAnonWalletStatus(callback);
}

void LedgerImpl::GetTransactionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  bat_database_->GetTransactionReport(month, year, callback);
}

void LedgerImpl::GetContributionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetContributionReportCallback callback) {
  bat_database_->GetContributionReport(month, year, callback);
}

void LedgerImpl::GetIncompleteContributions(
    const ledger::ContributionProcessor processor,
    ledger::ContributionInfoListCallback callback) {
  bat_database_->GetIncompleteContributions(processor, callback);
}

void LedgerImpl::GetContributionInfo(
    const std::string& contribution_id,
    ledger::GetContributionInfoCallback callback) {
  bat_database_->GetContributionInfo(contribution_id, callback);
}

void LedgerImpl::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
    const int32_t retry_count,
    ledger::ResultCallback callback) {
  bat_database_->UpdateContributionInfoStepAndCount(
      contribution_id,
      step,
      retry_count,
      callback);
}

void LedgerImpl::UpdateContributionInfoContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  bat_database_->UpdateContributionInfoContributedAmount(
      contribution_id,
      publisher_key,
      callback);
}

void LedgerImpl::RunDBTransaction(
    ledger::DBTransactionPtr transaction,
    ledger::RunDBTransactionCallback callback) {
  ledger_client_->RunDBTransaction(std::move(transaction), callback);
}

void LedgerImpl::GetCreateScript(
    ledger::GetCreateScriptCallback callback) {
  ledger_client_->GetCreateScript(callback);
}

void LedgerImpl::GetAllContributions(
    ledger::ContributionInfoListCallback callback) {
  bat_database_->GetAllContributions(callback);
}

void LedgerImpl::GetMonthlyReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetMonthlyReportCallback callback) {
  bat_report_->GetMonthly(month, year, callback);
}

void LedgerImpl::GetAllMonthlyReportIds(
    ledger::GetAllMonthlyReportIdsCallback callback) {
  bat_report_->GetAllMonthlyIds(callback);
}

void LedgerImpl::TransferTokens(
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  bat_promotion_->TransferTokens(std::move(wallet), callback);
}

void LedgerImpl::SaveCredsBatch(
    ledger::CredsBatchPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveCredsBatch(std::move(info), callback);
}

void LedgerImpl::SavePromotionClaimId(
    const std::string& promotion_id,
    const std::string& claim_id,
    ledger::ResultCallback callback) {
  bat_database_->SavePromotionClaimId(promotion_id, claim_id, callback);
}

void LedgerImpl::GetCredsBatchByTrigger(
    const std::string& trigger_id,
    const ledger::CredsBatchType trigger_type,
    ledger::GetCredsBatchCallback callback) {
  bat_database_->GetCredsBatchByTrigger(trigger_id, trigger_type, callback);
}

void LedgerImpl::SaveSignedCreds(
    ledger::CredsBatchPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveSignedCreds(std::move(info), callback);
}

void LedgerImpl::UpdatePromotionStatus(
    const std::string& promotion_id,
    const ledger::PromotionStatus status,
    ledger::ResultCallback callback) {
  bat_database_->UpdatePromotionStatus(promotion_id, status, callback);
}

void LedgerImpl::PromotionCredentialCompleted(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  bat_database_->PromotionCredentialCompleted(promotion_id, callback);
}

void LedgerImpl::GetAllCredsBatches(ledger::GetAllCredsBatchCallback callback) {
  bat_database_->GetAllCredsBatches(callback);
}

void LedgerImpl::GetPromotionList(
    const std::vector<std::string>& ids,
    ledger::GetPromotionListCallback callback) {
  bat_database_->GetPromotionList(ids, callback);
}

void LedgerImpl::GetPromotionListByType(
    const std::vector<ledger::PromotionType>& types,
    ledger::GetPromotionListCallback callback) {
  bat_database_->GetPromotionListByType(types, callback);
}

void LedgerImpl::CheckUnblindedTokensExpiration(
    ledger::ResultCallback callback) {
  bat_database_->CheckUnblindedTokensExpiration(callback);
}

void LedgerImpl::UpdateCredsBatchStatus(
    const std::string& trigger_id,
    const ledger::CredsBatchType trigger_type,
    const ledger::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  bat_database_->UpdateCredsBatchStatus(
      trigger_id,
      trigger_type,
      status,
      callback);
}

}  // namespace bat_ledger

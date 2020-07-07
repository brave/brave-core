/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <ctime>
#include <random>
#include <utility>
#include <vector>

#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "bat/ads/issuers_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/confirmations/confirmations.h"
#include "bat/ledger/internal/api/api.h"
#include "bat/ledger/internal/media/media.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/publisher/prefix_list_reader.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/publisher/publisher_status_helper.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/legacy/bat_state.h"
#include "bat/ledger/internal/promotion/promotion.h"
#include "bat/ledger/internal/recovery/recovery.h"
#include "bat/ledger/internal/report/report.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/sku/sku_factory.h"
#include "bat/ledger/internal/sku/sku_merchant.h"
#include "bat/ledger/internal/state/state.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_util.h"
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
using namespace braveledger_sku; //  NOLINT
using namespace braveledger_state; //  NOLINT
using namespace braveledger_api; //  NOLINT
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_3;

namespace bat_ledger {

LedgerImpl::LedgerImpl(ledger::LedgerClient* client) :
    ledger_client_(client),
    bat_promotion_(new Promotion(this)),
    bat_publisher_(new Publisher(this)),
    bat_media_(new Media(this)),
    legacy_bat_state_(new LegacyBatState(this)),
    bat_contribution_(new Contribution(this)),
    bat_wallet_(new Wallet(this)),
    bat_database_(new Database(this)),
    bat_report_(new Report(this)),
    bat_state_(new State(this)),
    bat_api_(new API(this)),
    initialized_task_scheduler_(false),
    initialized_(false),
    initializing_(false),
    last_tab_active_time_(0),
    last_shown_tab_id_(-1),
    last_pub_load_timer_id_(0u) {
  // Ensure ThreadPoolInstance is initialized before creating the task runner
  // for ios.
  set_ledger_client_for_logging(ledger_client_);

  if (!base::ThreadPoolInstance::Get()) {
    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("bat_ledger");

    DCHECK(base::ThreadPoolInstance::Get());
    initialized_task_scheduler_ = true;
  }

  task_runner_ = base::CreateSequencedTaskRunner(
      {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  bat_sku_ = braveledger_sku::SKUFactory::Create(
      this,
      braveledger_sku::SKUType::kMerchant);
  DCHECK(bat_sku_);
}

LedgerImpl::~LedgerImpl() {
  if (initialized_task_scheduler_) {
    DCHECK(base::ThreadPoolInstance::Get());
    base::ThreadPoolInstance::Get()->Shutdown();
  }
}

void LedgerImpl::OnInitialized(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  initializing_ = false;
  callback(result);
  if (result == ledger::Result::LEDGER_OK) {
    initialized_ = true;
    StartServices();
  } else {
    BLOG(0, "Failed to initialize wallet " << result);
  }
}

void LedgerImpl::StartServices() {
  if (!IsWalletCreated()) {
    return;
  }

  bat_publisher_->SetPublisherServerListTimer(GetRewardsMainEnabled());
  bat_contribution_->SetReconcileTimer();
  bat_promotion_->Refresh(false);
  bat_contribution_->Initialize();
  bat_promotion_->Initialize();
  bat_api_->Initialize();
  SetConfirmationsWalletInfo();
  braveledger_recovery::Check(this);
}

void LedgerImpl::Initialize(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  DCHECK(!initializing_);
  if (initializing_) {
    BLOG(1, "Already initializing ledger");
    return;
  }

  initializing_ = true;

  InitializeDatabase(execute_create_script, callback);
}

void LedgerImpl::InitializeDatabase(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  ledger::ResultCallback finish_callback =
      std::bind(&LedgerImpl::OnInitialized,
          this,
          _1,
          std::move(callback));

  auto database_callback = std::bind(&LedgerImpl::OnDatabaseInitialized,
      this,
      _1,
      finish_callback);
  bat_database_->Initialize(execute_create_script, database_callback);
}

void LedgerImpl::OnDatabaseInitialized(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Database could not be initialized. Error: " << result);
    callback(result);
    return;
  }

  auto state_callback = std::bind(&LedgerImpl::OnStateInitialized,
      this,
      _1,
      callback);

  bat_state_->Initialize(state_callback);
}

void LedgerImpl::OnStateInitialized(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Failed to initialize state");
    return;
  }

  MaybeInitializeConfirmations(callback);
}

void LedgerImpl::MaybeInitializeConfirmations(
    ledger::ResultCallback callback) {
  confirmations::_environment = ledger::_environment;
  confirmations::_is_debug = ledger::is_debug;

  const bool is_enabled = GetBooleanState(ledger::kStateEnabled);

  if (is_enabled) {
    InitializeConfirmations(callback);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void LedgerImpl::InitializeConfirmations(
    ledger::ResultCallback callback) {
  bat_confirmations_.reset(
      confirmations::Confirmations::CreateInstance(ledger_client_));

  auto initialized_callback = std::bind(&LedgerImpl::OnConfirmationsInitialized,
      this,
      _1,
      callback);
  bat_confirmations_->Initialize(initialized_callback);
}

void LedgerImpl::OnConfirmationsInitialized(
    const bool success,
    ledger::ResultCallback callback) {
  if (!success) {
    // If confirmations fail, we should fall-through and continue initializing
    // ledger
    BLOG(0, "Failed to initialize confirmations");
  }

  callback(ledger::Result::LEDGER_OK);
}

void LedgerImpl::StartConfirmations() {
  if (IsConfirmationsRunning()) {
    return;
  }

  bat_confirmations_.reset(
      confirmations::Confirmations::CreateInstance(ledger_client_));

  const auto callback = std::bind(&LedgerImpl::OnConfirmationsStarted,
      this,
      _1);
  bat_confirmations_->Initialize(callback);
}

void LedgerImpl::OnConfirmationsStarted(
    const bool success) {
  if (!success) {
    BLOG(0, "Failed to start confirmations");
    return;
  }

  BLOG(1, "Successfully started confirmations");

  SetConfirmationsWalletInfo();
}

void LedgerImpl::ShutdownConfirmations() {
  if (!IsConfirmationsRunning()) {
    return;
  }

  BLOG(1, "Successfully shutdown confirmations");
  bat_confirmations_.release();
}

bool LedgerImpl::IsConfirmationsRunning() {
  if (!bat_confirmations_ || !initialized_) {
    return false;
  }

  return true;
}

void LedgerImpl::CreateWallet(ledger::ResultCallback callback) {
  auto create_callback = std::bind(&LedgerImpl::OnCreateWallet,
      this,
      _1,
      callback);

  bat_wallet_->CreateWalletIfNecessary(create_callback);
}

void LedgerImpl::OnCreateWallet(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::WALLET_CREATED) {
    StartServices();
  }

  callback(result);
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
  if (!GetRewardsMainEnabled() || !GetAutoContributeEnabled()) {
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

void LedgerImpl::SetConfirmationsWalletInfo() {
  if (!IsConfirmationsRunning()) {
    return;
  }

  const auto recovery_seed = braveledger_state::GetRecoverySeed(this);
  const std::vector<uint8_t> seed =
      braveledger_bat_helper::getHKDF(recovery_seed);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> secret_key;

  if (!braveledger_bat_helper::getPublicKeyFromSeed(seed, &public_key,
      &secret_key)) {
    BLOG(0, "Failed to initialize confirmations due to invalid wallet");
    return;
  }

  confirmations::WalletInfo wallet_info;
  wallet_info.payment_id = GetPaymentId();
  wallet_info.private_key = braveledger_bat_helper::uint8ToHex(secret_key);

  if (!wallet_info.IsValid()) {
    BLOG(0, "Failed to initialize confirmations due to invalid wallet");
    return;
  }

  bat_confirmations_->SetWalletInfo(
      std::make_unique<confirmations::WalletInfo>(wallet_info));
}

void LedgerImpl::LoadPublisherState(ledger::OnLoadCallback callback) {
  ledger_client_->LoadPublisherState(std::move(callback));
}

void LedgerImpl::LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const ledger::UrlMethod method,
    ledger::LoadURLCallback callback) {
  if (shutting_down_) {
    BLOG(1,  url + " will not be executed as we are shutting down");
    return;
  }

  BLOG(5, ledger::UrlRequestToString(url, headers, content, content_type,
      method));

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
  if (!braveledger_state::GetPublisherAllowVideos(this)) {
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
  braveledger_state::SetRewardsMainEnabled(this, enabled);
  bat_publisher_->SetPublisherServerListTimer(enabled);

  if (enabled) {
    StartConfirmations();
  } else {
    ShutdownConfirmations();
  }
}

void LedgerImpl::SetPublisherMinVisitTime(int duration) {
  braveledger_state::SetPublisherMinVisitTime(this, duration);
}

void LedgerImpl::SetPublisherMinVisits(int visits) {
  braveledger_state::SetPublisherMinVisits(this, visits);
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  braveledger_state::SetPublisherAllowNonVerified(this, allow);
}

void LedgerImpl::SetPublisherAllowVideos(bool allow) {
  braveledger_state::SetPublisherAllowVideos(this, allow);
}

void LedgerImpl::SetAutoContributionAmount(double amount) {
  braveledger_state::SetAutoContributionAmount(this, amount);
}

void LedgerImpl::SetAutoContributeEnabled(bool enabled) {
  braveledger_state::SetAutoContributeEnabled(this, enabled);
}

ledger::AutoContributePropertiesPtr LedgerImpl::GetAutoContributeProperties() {
  auto props = ledger::AutoContributeProperties::New();
  props->enabled_contribute = GetAutoContributeEnabled();
  props->contribution_min_time = GetPublisherMinVisitTime();
  props->contribution_min_visits = GetPublisherMinVisits();
  props->contribution_non_verified = GetPublisherAllowNonVerified();
  props->contribution_videos = GetPublisherAllowVideos();
  props->reconcile_stamp = GetReconcileStamp();
  return props;
}

bool LedgerImpl::GetRewardsMainEnabled() {
  return braveledger_state::GetRewardsMainEnabled(this);
}

int LedgerImpl::GetPublisherMinVisitTime() {
  return braveledger_state::GetPublisherMinVisitTime(this);
}

int LedgerImpl::GetPublisherMinVisits() {
  return braveledger_state::GetPublisherMinVisits(this);
}

bool LedgerImpl::GetPublisherAllowNonVerified() {
  return braveledger_state::GetPublisherAllowNonVerified(this);
}

bool LedgerImpl::GetPublisherAllowVideos() {
  return braveledger_state::GetPublisherAllowVideos(this);
}

double LedgerImpl::GetAutoContributionAmount() {
  return braveledger_state::GetAutoContributionAmount(this);
}

bool LedgerImpl::GetAutoContributeEnabled() {
  return braveledger_state::GetAutoContributeEnabled(this);
}

uint64_t LedgerImpl::GetReconcileStamp() {
  return braveledger_state::GetReconcileStamp(this);
}

void LedgerImpl::ContributionCompleted(
    const ledger::Result result,
    ledger::ContributionInfoPtr contribution) {
  bat_contribution_->ContributionCompleted(
      result,
      contribution->Clone());

  // TODO(https://github.com/brave/brave-browser/issues/7717)
  // rename to ContributionCompleted
  ledger_client_->OnReconcileComplete(
      result,
      contribution->Clone());
}

void LedgerImpl::GetRewardsParameters(
    ledger::GetRewardsParametersCallback callback) {
  callback(braveledger_state::GetRewardsParameters(this));
}

void LedgerImpl::ClaimPromotion(
    const std::string& promotion_id,
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) const {
  bat_promotion_->Claim(promotion_id, payload, std::move(callback));
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
    BLOG(0, "Failed to recover wallet");
  }

  if (result == ledger::Result::LEDGER_OK) {
    bat_database_->DeleteAllBalanceReports([](const ledger::Result _) {});
  }

  callback(result, balance);
}

void LedgerImpl::GetBalanceReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetBalanceReportCallback callback) const {
  bat_database_->GetBalanceReportInfo(month, year, callback);
}

void LedgerImpl::GetAllBalanceReports(
    ledger::GetBalanceReportListCallback callback) const {
  return bat_database_->GetAllBalanceReports(callback);
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
  bat_promotion_->OnTimer(timer_id);
  bat_api_->OnTimer(timer_id);
}

void LedgerImpl::SaveRecurringTip(
    ledger::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  bat_database_->SaveRecurringTip(
      std::move(info),
      callback);
}

void LedgerImpl::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  bat_database_->GetRecurringTips([this, callback](
      ledger::PublisherInfoList list) {
    // The publisher status field may be expired. Attempt to refresh
    // expired publisher status values before executing callback.
    braveledger_publisher::RefreshPublisherStatus(
        this,
        std::move(list),
        callback);
  });
}

void LedgerImpl::GetOneTimeTips(ledger::PublisherInfoListCallback callback) {
  bat_database_->GetOneTimeTips(
      braveledger_time_util::GetCurrentMonth(),
      braveledger_time_util::GetCurrentYear(),
      callback);
}

bool LedgerImpl::IsWalletCreated() {
  const auto stamp = braveledger_state::GetCreationStamp(this);
  return stamp != 0u;
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
  bat_database_->SaveBalanceReportInfoItem(
      month,
      year,
      type,
      amount,
      [](const ledger::Result _) {});
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

void LedgerImpl::UpdateAdsRewards(const bool should_refresh) {
  if (!IsConfirmationsRunning()) {
    return;
  }

  bat_confirmations_->UpdateAdsRewards(should_refresh);
}

void LedgerImpl::ResetReconcileStamp() {
  braveledger_state::SetReconcileStamp(this, ledger::reconcile_interval);
  ledger_client_->ReconcileStampReset();
}

std::string LedgerImpl::GetPaymentId() {
  return braveledger_state::GetPaymentId(this);
}

void LedgerImpl::GetRewardsInternalsInfo(
    ledger::RewardsInternalsInfoCallback callback) {
  ledger::RewardsInternalsInfoPtr info = ledger::RewardsInternalsInfo::New();

  // Retrieve the payment id.
  info->payment_id = GetPaymentId();

  // Retrieve the boot stamp.
  info->boot_stamp = GetCreationStamp();

  // Retrieve the key info seed and validate it.
  const auto seed = braveledger_state::GetRecoverySeed(this);
  if (seed.size() != SEED_LENGTH) {
    info->is_key_info_seed_valid = false;
  } else {
    std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(seed);
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> new_secret_key;
    info->is_key_info_seed_valid = braveledger_bat_helper::getPublicKeyFromSeed(
        secret_key, &public_key, &new_secret_key);
  }

  callback(std::move(info));
}

void LedgerImpl::StartMonthlyContribution() {
  bat_contribution_->StartMonthlyContribution();
}

uint64_t LedgerImpl::GetCreationStamp() {
  return braveledger_state::GetCreationStamp(this);
}

void LedgerImpl::SetCreationStamp(uint64_t stamp) {
  braveledger_state::SetCreationStamp(this, stamp);
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

void LedgerImpl::HasSufficientBalanceToReconcile(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  bat_contribution_->HasSufficientBalance(callback);
}

void LedgerImpl::SaveNormalizedPublisherList(ledger::PublisherInfoList list) {
  ledger::PublisherInfoList save_list;
  for (auto& item : list) {
    save_list.push_back(item.Clone());
  }

  bat_database_->NormalizeActivityInfoList(
      std::move(save_list),
      [](const ledger::Result){});
  ledger_client_->PublisherListNormalized(std::move(list));
}

void LedgerImpl::SetCatalogIssuers(const std::string& info) {
  if (!IsConfirmationsRunning()) {
    return;
  }

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
  if (!IsConfirmationsRunning()) {
    return;
  }

  ads::AdInfo ad_info;
  if (ad_info.FromJson(json) != ads::Result::SUCCESS) {
    return;
  }

  confirmations::AdInfo confirmations_ad_info;
  confirmations_ad_info.creative_instance_id = ad_info.creative_instance_id;
  confirmations_ad_info.creative_set_id = ad_info.creative_set_id;
  confirmations_ad_info.category = ad_info.category;
  confirmations_ad_info.target_url = ad_info.target_url;
  confirmations_ad_info.geo_target = ad_info.geo_target;

  bat_confirmations_->ConfirmAd(confirmations_ad_info,
      confirmations::ConfirmationType(confirmation_type));
}

void LedgerImpl::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const std::string& confirmation_type) {
  if (!IsConfirmationsRunning()) {
    return;
  }

  bat_confirmations_->ConfirmAction(creative_instance_id, creative_set_id,
      confirmations::ConfirmationType(confirmation_type));
}

void LedgerImpl::GetTransactionHistory(
    ledger::GetTransactionHistoryCallback callback) {
  if (!IsConfirmationsRunning()) {
    return;
  }

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

void LedgerImpl::SetInlineTippingPlatformEnabled(
    const ledger::InlineTipsPlatforms platform,
    bool enabled) {
  braveledger_state::SetInlineTippingPlatformEnabled(this, platform, enabled);
}

bool LedgerImpl::GetInlineTippingPlatformEnabled(
    const ledger::InlineTipsPlatforms platform) {
  return braveledger_state::GetInlineTippingPlatformEnabled(this, platform);
}

std::string LedgerImpl::GetShareURL(
    const std::string& type,
    const std::map<std::string, std::string>& args) {
  return bat_media_->GetShareURL(type, args);
}

void LedgerImpl::GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) {
  bat_database_->GetPendingContributions([this, callback](
      ledger::PendingContributionInfoList list) {
    // The publisher status field may be expired. Attempt to refresh
    // expired publisher status values before executing callback.
    braveledger_publisher::RefreshPublisherStatus(
        this,
        std::move(list),
        callback);
  });
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

void LedgerImpl::WasPublisherProcessed(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  bat_database_->WasPublisherProcessed(publisher_key, callback);
}

void LedgerImpl::FetchBalance(ledger::FetchBalanceCallback callback) {
  bat_wallet_->FetchBalance(callback);
}

std::map<std::string, ledger::ExternalWalletPtr>
LedgerImpl::GetExternalWallets() {
  return ledger_client_->GetExternalWallets();
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
    ledger::ResultCallback callback,
    const bool allow_zero_balance) {
  bat_wallet_->TransferAnonToExternalWallet(allow_zero_balance, callback);
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

bool LedgerImpl::ShouldFetchServerPublisherInfo(
    ledger::ServerPublisherInfo* server_info) {
  return bat_publisher_->ShouldFetchServerPublisherInfo(server_info);
}

void LedgerImpl::SearchPublisherPrefixList(
    const std::string& publisher_key,
    ledger::SearchPublisherPrefixListCallback callback) {
  bat_database_->SearchPublisherPrefixList(publisher_key, callback);
}

void LedgerImpl::ResetPublisherPrefixList(
    std::unique_ptr<braveledger_publisher::PrefixListReader> reader,
    ledger::ResultCallback callback) {
  bat_database_->ResetPublisherPrefixList(std::move(reader), callback);
}

void LedgerImpl::InsertServerPublisherInfo(
    const ledger::ServerPublisherInfo& server_info,
    ledger::ResultCallback callback) {
  bat_database_->InsertServerPublisherInfo(server_info, callback);
}

void LedgerImpl::GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  bat_database_->GetServerPublisherInfo(
      publisher_key,
      std::bind(&LedgerImpl::OnServerPublisherInfoLoaded,
          this, _1, publisher_key, callback));
}

void LedgerImpl::OnServerPublisherInfoLoaded(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  if (ShouldFetchServerPublisherInfo(server_info.get())) {
    BLOG(1, "Server publisher info  is expired for " << publisher_key);
    bat_publisher_->FetchServerPublisherInfo(publisher_key, callback);
  } else {
    callback(std::move(server_info));
  }
}

void LedgerImpl::DeleteExpiredServerPublisherInfo(
    const int64_t max_age_seconds,
    ledger::ResultCallback callback) {
  bat_database_->DeleteExpiredServerPublisherInfo(max_age_seconds, callback);
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

void LedgerImpl::MarkContributionQueueAsComplete(
    const std::string& id,
    ledger::ResultCallback callback) {
  bat_database_->MarkContributionQueueAsComplete(id, callback);
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

void LedgerImpl::SaveUnblindedTokenList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  bat_database_->SaveUnblindedTokenList(std::move(list), callback);
}

void LedgerImpl::MarkUnblindedTokensAsSpent(
    const std::vector<std::string>& ids,
    ledger::RewardsType redeem_type,
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  bat_database_->MarkUnblindedTokensAsSpent(
      ids,
      redeem_type,
      redeem_id,
      callback);
}

void LedgerImpl::MarkUnblindedTokensAsReserved(
    const std::vector<std::string>& ids,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  bat_database_->MarkUnblindedTokensAsReserved(ids, contribution_id, callback);
}

void LedgerImpl::MarkUnblindedTokensAsSpendable(
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  bat_database_->MarkUnblindedTokensAsSpendable(contribution_id, callback);
}

void LedgerImpl::GetSpendableUnblindedTokensByTriggerIds(
    const std::vector<std::string>& trigger_ids,
    ledger::GetUnblindedTokenListCallback callback) {
  bat_database_->GetSpendableUnblindedTokensByTriggerIds(trigger_ids, callback);
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

void LedgerImpl::GetNotCompletedContributions(
    ledger::ContributionInfoListCallback callback) {
  bat_database_->GetNotCompletedContributions(callback);
}

void LedgerImpl::GetContributionInfo(
    const std::string& contribution_id,
    ledger::GetContributionInfoCallback callback) {
  bat_database_->GetContributionInfo(contribution_id, callback);
}

void LedgerImpl::UpdateContributionInfoStep(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
    ledger::ResultCallback callback) {
  bat_database_->UpdateContributionInfoStep(
      contribution_id,
      step,
      callback);
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

void LedgerImpl::TransferTokens(ledger::ResultCallback callback) {
  bat_promotion_->TransferTokens(callback);
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

void LedgerImpl::UpdatePromotionsStatus(
    const std::vector<std::string>& promotion_ids,
    const ledger::PromotionStatus status,
    ledger::ResultCallback callback) {
  bat_database_->UpdatePromotionsStatus(promotion_ids, status, callback);
}

void LedgerImpl::PromotionCredentialCompleted(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  bat_database_->PromotionCredentialCompleted(promotion_id, callback);
}

void LedgerImpl::GetAllCredsBatches(
    ledger::GetCredsBatchListCallback callback) {
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

void LedgerImpl::UpdateCredsBatchesStatus(
    const std::vector<std::string>& trigger_ids,
    const ledger::CredsBatchType trigger_type,
    const ledger::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  bat_database_->UpdateCredsBatchesStatus(
      trigger_ids,
      trigger_type,
      status,
      callback);
}

void LedgerImpl::SaveSKUOrder(
    ledger::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  bat_database_->SaveSKUOrder(std::move(order), callback);
}

void LedgerImpl::SaveSKUTransaction(
    ledger::SKUTransactionPtr transaction,
    ledger::ResultCallback callback) {
  bat_database_->SaveSKUTransaction(std::move(transaction), callback);
}

void LedgerImpl::SaveSKUExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ledger::ResultCallback callback) {
  bat_database_->SaveSKUExternalTransaction(
      transaction_id,
      external_transaction_id,
      callback);
}

void LedgerImpl::UpdateSKUOrderStatus(
    const std::string& order_id,
    const ledger::SKUOrderStatus status,
    ledger::ResultCallback callback) {
  bat_database_->UpdateSKUOrderStatus(
      order_id,
      status,
      callback);
}

void LedgerImpl::TransferFunds(
    const ledger::SKUTransaction& transaction,
    const std::string& destination,
    ledger::ExternalWalletPtr wallet,
    ledger::TransactionCallback callback) {
  bat_contribution_->TransferFunds(
      transaction,
      destination,
      std::move(wallet),
      callback);
}

void LedgerImpl::GetSKUOrder(
    const std::string& order_id,
    ledger::GetSKUOrderCallback callback) {
  bat_database_->GetSKUOrder(order_id, callback);
}

void LedgerImpl::ProcessSKU(
    const std::vector<ledger::SKUOrderItem>& items,
    ledger::ExternalWalletPtr wallet,
    ledger::SKUOrderCallback callback) {
  bat_sku_->Process(items, std::move(wallet), callback);
}

void LedgerImpl::GetSKUOrderByContributionId(
    const std::string& contribution_id,
    ledger::GetSKUOrderCallback callback) {
  bat_database_->GetSKUOrderByContributionId(contribution_id, callback);
}

void LedgerImpl::SaveContributionIdForSKUOrder(
    const std::string& order_id,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  bat_database_->SaveContributionIdForSKUOrder(
      order_id,
      contribution_id,
      callback);
}

void LedgerImpl::GetSKUTransactionByOrderId(
    const std::string& order_id,
    ledger::GetSKUTransactionCallback callback) {
  bat_database_->GetSKUTransactionByOrderId(order_id, callback);
}

void LedgerImpl::GetReservedUnblindedTokens(
    const std::string& redeem_id,
    ledger::GetUnblindedTokenListCallback callback) {
  bat_database_->GetReservedUnblindedTokens(
      redeem_id,
      callback);
}

void LedgerImpl::GetSpendableUnblindedTokensByBatchTypes(
    const std::vector<ledger::CredsBatchType>& batch_types,
    ledger::GetUnblindedTokenListCallback callback) {
  bat_database_->GetSpendableUnblindedTokensByBatchTypes(
      batch_types,
      callback);
}

void LedgerImpl::UpdatePromotionsBlankPublicKey(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  bat_database_->UpdatePromotionsBlankPublicKey(ids, callback);
}

void LedgerImpl::SynopsisNormalizer() {
  bat_publisher_->SynopsisNormalizer();
}

void LedgerImpl::CalcScoreConsts(const int min_duration_seconds) {
  bat_publisher_->CalcScoreConsts(min_duration_seconds);
}

void LedgerImpl::SaveBalanceReportInfoList(
    ledger::BalanceReportInfoList list,
    ledger::ResultCallback callback) {
  bat_database_->SaveBalanceReportInfoList(std::move(list), callback);
}

void LedgerImpl::SaveProcessedPublisherList(
    const std::vector<std::string>& list,
    ledger::ResultCallback callback) {
  bat_database_->SaveProcessedPublisherList(list, callback);
}

void LedgerImpl::FetchParameters() {
  bat_api_->FetchParameters();
}

void LedgerImpl::Shutdown(ledger::ResultCallback callback) {
  shutting_down_ = true;
  ledger_client_->ClearAllNotifications();

  auto disconnect_callback = std::bind(&LedgerImpl::ShutdownWallets,
      this,
      _1,
      callback);

  bat_wallet_->DisconnectAllWallets(disconnect_callback);
}

void LedgerImpl::ShutdownWallets(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  BLOG_IF(
      1,
      result != ledger::Result::LEDGER_OK,
      "Not all wallets were disconnected");
  bat_database_->FinishAllInProgressContributions(callback);
}

void LedgerImpl::GetCredsBatchesByTriggers(
    const std::vector<std::string>& trigger_ids,
    ledger::GetCredsBatchListCallback callback) {
  bat_database_->GetCredsBatchesByTriggers(trigger_ids, callback);
}

}  // namespace bat_ledger

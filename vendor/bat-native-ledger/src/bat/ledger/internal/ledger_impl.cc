/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "bat/ledger/internal/publisher/publisher_status_helper.h"
#include "bat/ledger/internal/sku/sku_factory.h"
#include "bat/ledger/internal/sku/sku_merchant.h"

using std::placeholders::_1;

namespace ledger {

LedgerImpl::LedgerImpl(ledger::LedgerClient* client)
    : ledger_client_(client),
      context_(std::make_unique<BATLedgerContext>(this)),
      promotion_(std::make_unique<promotion::Promotion>(this)),
      publisher_(std::make_unique<publisher::Publisher>(this)),
      media_(std::make_unique<braveledger_media::Media>(this)),
      contribution_(std::make_unique<contribution::Contribution>(this)),
      wallet_(std::make_unique<wallet::Wallet>(this)),
      database_(std::make_unique<database::Database>(this)),
      report_(std::make_unique<report::Report>(this)),
      state_(std::make_unique<state::State>(this)),
      api_(std::make_unique<api::API>(this)),
      recovery_(std::make_unique<recovery::Recovery>(this)),
      bitflyer_(std::make_unique<bitflyer::Bitflyer>(this)),
      uphold_(std::make_unique<uphold::Uphold>(this)),
      initialized_task_scheduler_(false),
      initializing_(false),
      last_tab_active_time_(0),
      last_shown_tab_id_(-1) {
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

  sku_ = sku::SKUFactory::Create(
      this,
      sku::SKUType::kMerchant);
  DCHECK(sku_);
}

LedgerImpl::~LedgerImpl() {
  if (initialized_task_scheduler_) {
    DCHECK(base::ThreadPoolInstance::Get());
    base::ThreadPoolInstance::Get()->Shutdown();
  }
}

BATLedgerContext* LedgerImpl::context() const {
  return context_.get();
}

ledger::LedgerClient* LedgerImpl::ledger_client() const {
  return ledger_client_;
}

state::State* LedgerImpl::state() const {
  return state_.get();
}

promotion::Promotion* LedgerImpl::promotion() const {
  return promotion_.get();
}

publisher::Publisher* LedgerImpl::publisher() const {
  return publisher_.get();
}

braveledger_media::Media* LedgerImpl::media() const {
  return media_.get();
}

contribution::Contribution* LedgerImpl::contribution() const {
  return contribution_.get();
}

wallet::Wallet* LedgerImpl::wallet() const {
  return wallet_.get();
}

report::Report* LedgerImpl::report() const {
  return report_.get();
}

sku::SKU* LedgerImpl::sku() const {
  return sku_.get();
}

api::API* LedgerImpl::api() const {
  return api_.get();
}

database::Database* LedgerImpl::database() const {
  return database_.get();
}

bitflyer::Bitflyer* LedgerImpl::bitflyer() const {
  return bitflyer_.get();
}

uphold::Uphold* LedgerImpl::uphold() const {
  return uphold_.get();
}

void LedgerImpl::LoadURL(
    type::UrlRequestPtr request,
    client::LoadURLCallback callback) {
  DCHECK(request);
  if (shutting_down_) {
    BLOG(1, request->url + " will not be executed as we are shutting down");
    return;
  }

  if (!request->skip_log) {
    BLOG(5, ledger::UrlRequestToString(
        request->url,
        request->headers,
        request->content,
        request->content_type,
        request->method));
  }

  ledger_client_->LoadURL(std::move(request), callback);
}

void LedgerImpl::StartServices() {
  publisher()->SetPublisherServerListTimer();
  contribution()->SetReconcileTimer();
  promotion()->Refresh(false);
  contribution()->Initialize();
  promotion()->Initialize();
  api()->Initialize();
  recovery_->Check();
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
  ledger::ResultCallback finish_callback = std::bind(
      &LedgerImpl::OnInitialized,
      this,
      _1,
      std::move(callback));

  auto database_callback = std::bind(&LedgerImpl::OnDatabaseInitialized,
      this,
      _1,
      finish_callback);
  database()->Initialize(execute_create_script, database_callback);
}

void LedgerImpl::OnInitialized(
    const type::Result result,
    ledger::ResultCallback callback) {
  initializing_ = false;

  if (result == type::Result::LEDGER_OK) {
    StartServices();
  } else {
    BLOG(0, "Failed to initialize wallet " << result);
  }

  callback(result);
}

void LedgerImpl::OnDatabaseInitialized(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Database could not be initialized. Error: " << result);
    callback(result);
    return;
  }

  auto state_callback = std::bind(&LedgerImpl::OnStateInitialized,
      this,
      _1,
      callback);

  state()->Initialize(state_callback);
}

void LedgerImpl::OnStateInitialized(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to initialize state");
    return;
  }

  callback(type::Result::LEDGER_OK);
}

void LedgerImpl::CreateWallet(ledger::ResultCallback callback) {
  wallet()->CreateWalletIfNecessary(callback);
}

void LedgerImpl::OneTimeTip(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  contribution()->OneTimeTip(publisher_key, amount, callback);
}

void LedgerImpl::OnLoad(
    type::VisitDataPtr visit_data,
    const uint64_t& current_time) {
  if (!visit_data.get()) {
    return;
  }

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
  if (!state()->GetAutoContributeEnabled()) {
    return;
  }

  if (tab_id != last_shown_tab_id_ || last_tab_active_time_ == 0) {
    return;
  }

  visit_data_iter iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end()) {
    return;
  }

  const std::string type = media()->GetLinkType(iter->second.tld, "", "");
  const auto duration = current_time - last_tab_active_time_;
  last_tab_active_time_ = 0;

  if (type == GITHUB_MEDIA_TYPE) {
      base::flat_map<std::string, std::string> parts;
      parts["duration"] = std::to_string(duration);
      media()->ProcessMedia(parts, type, iter->second.Clone());
    return;
  }

  publisher()->SaveVisit(
      iter->second.tld,
      iter->second,
      duration,
      true,
      0,
      [](type::Result, type::PublisherInfoPtr){});
}

void LedgerImpl::OnForeground(uint32_t tab_id, const uint64_t& current_time) {
  if (last_shown_tab_id_ != tab_id) {
    return;
  }
  OnShow(tab_id, current_time);
}

void LedgerImpl::OnBackground(uint32_t tab_id, const uint64_t& current_time) {
  OnHide(tab_id, current_time);
}

void LedgerImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    type::VisitDataPtr visit_data) {
  std::string type = media()->GetLinkType(
      url,
      first_party_url,
      referrer);

  if (type.empty()) {
    // It is not a media supported type
    return;
  }
  media()->ProcessMedia(parts, type, std::move(visit_data));
}

void LedgerImpl::OnPostData(
    const std::string& url,
    const std::string& first_party_url,
    const std::string& referrer,
    const std::string& post_data,
    type::VisitDataPtr visit_data) {
  std::string type = media()->GetLinkType(
      url,
      first_party_url,
      referrer);

  if (type.empty()) {
     // It is not a media supported type
    return;
  }

  if (type == TWITCH_MEDIA_TYPE) {
    std::vector<base::flat_map<std::string, std::string>> twitchParts;
    braveledger_media::GetTwitchParts(post_data, &twitchParts);
    for (size_t i = 0; i < twitchParts.size(); i++) {
      media()->ProcessMedia(twitchParts[i], type, std::move(visit_data));
    }
    return;
  }

  if (type == VIMEO_MEDIA_TYPE) {
    std::vector<base::flat_map<std::string, std::string>> parts;
    braveledger_media::GetVimeoParts(post_data, &parts);

    for (auto part = parts.begin(); part != parts.end(); part++) {
      media()->ProcessMedia(*part, type, std::move(visit_data));
    }
    return;
  }
}

std::string LedgerImpl::URIEncode(const std::string& value) {
  return ledger_client_->URIEncode(value);
}

void LedgerImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    type::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  database()->GetActivityInfoList(
      start,
      limit,
      std::move(filter),
      callback);
}

void LedgerImpl::GetExcludedList(ledger::PublisherInfoListCallback callback) {
  database()->GetExcludedList(callback);
}

void LedgerImpl::SetPublisherMinVisitTime(int duration) {
  state()->SetPublisherMinVisitTime(duration);
}

void LedgerImpl::SetPublisherMinVisits(int visits) {
  state()->SetPublisherMinVisits(visits);
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  state()->SetPublisherAllowNonVerified(allow);
}

void LedgerImpl::SetPublisherAllowVideos(bool allow) {
  state()->SetPublisherAllowVideos(allow);
}

void LedgerImpl::SetAutoContributionAmount(double amount) {
  state()->SetAutoContributionAmount(amount);
}

void LedgerImpl::SetAutoContributeEnabled(bool enabled) {
  state()->SetAutoContributeEnabled(enabled);
}

uint64_t LedgerImpl::GetReconcileStamp() {
  return state()->GetReconcileStamp();
}

int LedgerImpl::GetPublisherMinVisitTime() {
  return state()->GetPublisherMinVisitTime();
}

int LedgerImpl::GetPublisherMinVisits() {
  return state()->GetPublisherMinVisits();
}

bool LedgerImpl::GetPublisherAllowNonVerified() {
  return state()->GetPublisherAllowNonVerified();
}

bool LedgerImpl::GetPublisherAllowVideos() {
  return state()->GetPublisherAllowVideos();
}

double LedgerImpl::GetAutoContributionAmount() {
  return state()->GetAutoContributionAmount();
}

bool LedgerImpl::GetAutoContributeEnabled() {
  return state()->GetAutoContributeEnabled();
}

void LedgerImpl::GetRewardsParameters(
    ledger::GetRewardsParametersCallback callback) {
  auto params = state()->GetRewardsParameters();
  if (params->rate == 0.0) {
    // A rate of zero indicates that the rewards parameters have
    // not yet been successfully initialized from the server.
    BLOG(1, "Rewards parameters not set - fetching from server");
    api()->FetchParameters(callback);
    return;
  }

  callback(std::move(params));
}

void LedgerImpl::FetchPromotions(
    ledger::FetchPromotionCallback callback) const {
  promotion()->Fetch(callback);
}

void LedgerImpl::ClaimPromotion(
    const std::string& promotion_id,
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) const {
  promotion()->Claim(promotion_id, payload, std::move(callback));
}

void LedgerImpl::AttestPromotion(
    const std::string& promotion_id,
    const std::string& solution,
    ledger::AttestPromotionCallback callback) const {
  promotion()->Attest(promotion_id, solution, callback);
}

void LedgerImpl::GetBalanceReport(
    const type::ActivityMonth month,
    const int year,
    ledger::GetBalanceReportCallback callback) const {
  database()->GetBalanceReportInfo(month, year, callback);
}

void LedgerImpl::GetAllBalanceReports(
    ledger::GetBalanceReportListCallback callback) const {
  database()->GetAllBalanceReports(callback);
}

type::AutoContributePropertiesPtr LedgerImpl::GetAutoContributeProperties() {
  auto props = type::AutoContributeProperties::New();
  props->enabled_contribute = state()->GetAutoContributeEnabled();
  props->amount = state()->GetAutoContributionAmount();
  props->contribution_min_time = state()->GetPublisherMinVisitTime();
  props->contribution_min_visits = state()->GetPublisherMinVisits();
  props->contribution_non_verified = state()->GetPublisherAllowNonVerified();
  props->contribution_videos = state()->GetPublisherAllowVideos();
  props->reconcile_stamp = state()->GetReconcileStamp();
  return props;
}

void LedgerImpl::RecoverWallet(
    const std::string& pass_phrase,
    ledger::ResultCallback callback) {
  wallet()->RecoverWallet(pass_phrase, callback);
}

void LedgerImpl::SetPublisherExclude(
    const std::string& publisher_id,
    const type::PublisherExclude& exclude,
    ledger::ResultCallback callback) {
  publisher()->SetPublisherExclude(publisher_id, exclude, callback);
}

void LedgerImpl::RestorePublishers(ledger::ResultCallback callback) {
  database()->RestorePublishers(callback);
}

void LedgerImpl::GetPublisherActivityFromUrl(
    uint64_t windowId,
    type::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  publisher()->GetPublisherActivityFromUrl(
      windowId,
      std::move(visit_data),
      publisher_blob);
}

void LedgerImpl::GetPublisherBanner(
    const std::string& publisher_id,
    ledger::PublisherBannerCallback callback) {
  publisher()->GetPublisherBanner(publisher_id, callback);
}

void LedgerImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  database()->RemoveRecurringTip(publisher_key, callback);
}

uint64_t LedgerImpl::GetCreationStamp() {
  return state()->GetCreationStamp();
}

void LedgerImpl::HasSufficientBalanceToReconcile(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  contribution()->HasSufficientBalance(callback);
}

void LedgerImpl::GetRewardsInternalsInfo(
    ledger::RewardsInternalsInfoCallback callback) {
  auto info = type::RewardsInternalsInfo::New();

  type::BraveWalletPtr wallet = wallet_->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(std::move(info));
    return;
  }

  // Retrieve the payment id.
  info->payment_id = wallet->payment_id;

  // Retrieve the boot stamp.
  info->boot_stamp = state()->GetCreationStamp();

  // Retrieve the key info seed and validate it.
  if (!util::Security::IsSeedValid(wallet->recovery_seed)) {
    info->is_key_info_seed_valid = false;
  } else {
    std::vector<uint8_t> secret_key =
        util::Security::GetHKDF(wallet->recovery_seed);
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> new_secret_key;
    info->is_key_info_seed_valid =
        util::Security::GetPublicKeyFromSeed(
            secret_key,
            &public_key,
            &new_secret_key);
  }

  callback(std::move(info));
}

void LedgerImpl::SaveRecurringTip(
    type::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  database()->SaveRecurringTip(std::move(info), callback);
}

void LedgerImpl::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  contribution()->GetRecurringTips(callback);
}

void LedgerImpl::GetOneTimeTips(ledger::PublisherInfoListCallback callback) {
  database()->GetOneTimeTips(
      util::GetCurrentMonth(),
      util::GetCurrentYear(),
      callback);
}

void LedgerImpl::RefreshPublisher(
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  publisher()->RefreshPublisher(publisher_key, callback);
}

void LedgerImpl::StartMonthlyContribution() {
  contribution()->StartMonthlyContribution();
}

void LedgerImpl::SaveMediaInfo(
    const std::string& type,
    const base::flat_map<std::string, std::string>& data,
    ledger::PublisherInfoCallback callback) {
  media()->SaveMediaInfo(type, data, callback);
}

void LedgerImpl::UpdateMediaDuration(
    const uint64_t window_id,
    const std::string& publisher_key,
    const uint64_t duration,
    const bool first_visit) {
  publisher()->UpdateMediaDuration(
      window_id,
      publisher_key,
      duration,
      first_visit);
}

void LedgerImpl::GetPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  database()->GetPublisherInfo(publisher_key, callback);
}

void LedgerImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  publisher()->GetPublisherPanelInfo(publisher_key, callback);
}

void LedgerImpl::SavePublisherInfo(
    const uint64_t window_id,
    type::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  publisher()->SavePublisherInfo(
      window_id,
      std::move(publisher_info),
      callback);
}

void LedgerImpl::SetInlineTippingPlatformEnabled(
    const type::InlineTipsPlatforms platform,
    bool enabled) {
  state()->SetInlineTippingPlatformEnabled(platform, enabled);
}

bool LedgerImpl::GetInlineTippingPlatformEnabled(
    const type::InlineTipsPlatforms platform) {
  return state()->GetInlineTippingPlatformEnabled(platform);
}

std::string LedgerImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args) {
  return publisher()->GetShareURL(args);
}

void LedgerImpl::GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) {
  database()->GetPendingContributions([this, callback](
      type::PendingContributionInfoList list) {
    // The publisher status field may be expired. Attempt to refresh
    // expired publisher status values before executing callback.
    publisher::RefreshPublisherStatus(
        this,
        std::move(list),
        callback);
  });
}

void LedgerImpl::RemovePendingContribution(
    const uint64_t id,
    ledger::ResultCallback callback) {
  database()->RemovePendingContribution(id, callback);
}

void LedgerImpl::RemoveAllPendingContributions(
    ledger::ResultCallback callback) {
  database()->RemoveAllPendingContributions(callback);
}

void LedgerImpl::GetPendingContributionsTotal(
    ledger::PendingContributionsTotalCallback callback) {
  database()->GetPendingContributionsTotal(callback);
}

void LedgerImpl::FetchBalance(ledger::FetchBalanceCallback callback) {
  wallet()->FetchBalance(callback);
}

void LedgerImpl::GetExternalWallet(const std::string& wallet_type,
                                   ledger::ExternalWalletCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    uphold()->GenerateWallet([this, callback](const type::Result result) {
      if (result != type::Result::LEDGER_OK &&
          result != type::Result::CONTINUE) {
        callback(result, nullptr);
        return;
      }

      auto wallet = uphold()->GetWallet();
      callback(type::Result::LEDGER_OK, std::move(wallet));
    });
    return;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    bitflyer()->GenerateWallet([this, callback](const type::Result result) {
      if (result != type::Result::LEDGER_OK &&
          result != type::Result::CONTINUE) {
        callback(result, nullptr);
        return;
      }

      auto wallet = bitflyer()->GetWallet();
      callback(type::Result::LEDGER_OK, std::move(wallet));
    });
    return;
  }

  NOTREACHED();
  callback(type::Result::LEDGER_OK, nullptr);
}

void LedgerImpl::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  wallet()->ExternalWalletAuthorization(
      wallet_type,
      args,
      callback);
}

void LedgerImpl::DisconnectWallet(
    const std::string& wallet_type,
    ledger::ResultCallback callback) {
  wallet()->DisconnectWallet(wallet_type, callback);
}

void LedgerImpl::GetAllPromotions(
    ledger::GetAllPromotionsCallback callback) {
  database()->GetAllPromotions(callback);
}

void LedgerImpl::GetAnonWalletStatus(ledger::ResultCallback callback) {
  wallet()->GetAnonWalletStatus(callback);
}

void LedgerImpl::GetTransactionReport(
    const type::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  database()->GetTransactionReport(month, year, callback);
}

void LedgerImpl::GetContributionReport(
    const type::ActivityMonth month,
    const int year,
    ledger::GetContributionReportCallback callback) {
  database()->GetContributionReport(month, year, callback);
}

void LedgerImpl::GetAllContributions(
    ledger::ContributionInfoListCallback callback) {
  database()->GetAllContributions(callback);
}

void LedgerImpl::SavePublisherInfoForTip(
    type::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  database()->SavePublisherInfo(std::move(info), callback);
}

void LedgerImpl::GetMonthlyReport(
    const type::ActivityMonth month,
    const int year,
    ledger::GetMonthlyReportCallback callback) {
  report()->GetMonthly(month, year, callback);
}

void LedgerImpl::GetAllMonthlyReportIds(
    ledger::GetAllMonthlyReportIdsCallback callback) {
  report()->GetAllMonthlyIds(callback);
}

void LedgerImpl::ProcessSKU(
    const std::vector<type::SKUOrderItem>& items,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback) {
  sku()->Process(items, wallet_type, callback);
}

void LedgerImpl::Shutdown(ledger::ResultCallback callback) {
  shutting_down_ = true;
  ledger_client_->ClearAllNotifications();

  wallet()->DisconnectAllWallets([this, callback](
      const type::Result result){
    BLOG_IF(
      1,
      result != type::Result::LEDGER_OK,
      "Not all wallets were disconnected");
    auto finish_callback = std::bind(&LedgerImpl::OnAllDone,
        this,
        _1,
        callback);
    database()->FinishAllInProgressContributions(finish_callback);
  });
}

void LedgerImpl::OnAllDone(
    const type::Result result,
    ledger::ResultCallback callback) {
  database()->Close(callback);
}

void LedgerImpl::GetEventLogs(ledger::GetEventLogsCallback callback) {
  database()->GetLastEventLogs(callback);
}

bool LedgerImpl::IsShuttingDown() const {
  return shutting_down_;
}

void LedgerImpl::GetBraveWallet(GetBraveWalletCallback callback) {
  callback(wallet()->GetWallet());
}

std::string LedgerImpl::GetWalletPassphrase() const {
  const auto brave_wallet = wallet()->GetWallet();
  if (!brave_wallet) {
    return "";
  }

  return wallet()->GetWalletPassphrase(brave_wallet->Clone());
}

void LedgerImpl::LinkBraveWallet(
    const std::string& destination_payment_id,
    ResultCallback callback) {
  wallet()->LinkBraveWallet(destination_payment_id, callback);
}

void LedgerImpl::GetTransferableAmount(GetTransferableAmountCallback callback) {
  promotion()->GetTransferableAmount(callback);
}

}  // namespace ledger

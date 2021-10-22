/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

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

LedgerImpl::LedgerImpl(LedgerClient* client)
    : ledger_client_(client),
      context_(std::make_unique<BATLedgerContext>(this)),
      promotion_(std::make_unique<promotion::Promotion>(this)),
      publisher_(std::make_unique<publisher::Publisher>(this)),
      media_(std::make_unique<braveledger_media::Media>(this)),
      contribution_(std::make_unique<contribution::Contribution>(this)),
      wallet_(std::make_unique<wallet::Wallet>(this)),
      database_(std::make_unique<database::Database>(this)),
      report_(std::make_unique<report::Report>(this)),
      sku_(sku::SKUFactory::Create(this, sku::SKUType::kMerchant)),
      state_(std::make_unique<state::State>(this)),
      api_(std::make_unique<api::API>(this)),
      recovery_(std::make_unique<recovery::Recovery>(this)),
      bitflyer_(std::make_unique<bitflyer::Bitflyer>(this)),
      gemini_(std::make_unique<gemini::Gemini>(this)),
      uphold_(std::make_unique<uphold::Uphold>(this)),
      backup_restore_(std::make_unique<vg::BackupRestore>(this)) {
  DCHECK(base::ThreadPoolInstance::Get());
  set_ledger_client_for_logging(ledger_client_);
}

LedgerImpl::~LedgerImpl() = default;

BATLedgerContext* LedgerImpl::context() const {
  return context_.get();
}

LedgerClient* LedgerImpl::ledger_client() const {
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

gemini::Gemini* LedgerImpl::gemini() const {
  return gemini_.get();
}

uphold::Uphold* LedgerImpl::uphold() const {
  return uphold_.get();
}

vg::BackupRestore* LedgerImpl::backup_restore() const {
  return backup_restore_.get();
}

void LedgerImpl::LoadURL(
    type::UrlRequestPtr request,
    client::LoadURLCallback callback) {
  DCHECK(request);
  if (IsShuttingDown()) {
    BLOG(1, request->url + " will not be executed as we are shutting down");
    return;
  }

  if (!request->skip_log) {
    BLOG(5, UrlRequestToString(request->url, request->headers, request->content,
                               request->content_type, request->method));
  }

  ledger_client_->LoadURL(std::move(request), callback);
}

void LedgerImpl::StartServices() {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  publisher()->SetPublisherServerListTimer();
  contribution()->SetReconcileTimer();
  promotion()->Refresh(false);
  contribution()->Initialize();
  promotion()->Initialize();
  api()->Initialize();
  recovery_->Check();
  backup_restore()->StartBackUpVGSpendStatus();
}

void LedgerImpl::Initialize(bool execute_create_script,
                            ResultCallback callback) {
  if (ready_state_ != ReadyState::kUninitialized) {
    BLOG(0, "Ledger already initializing");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  ready_state_ = ReadyState::kInitializing;
  InitializeDatabase(execute_create_script, callback);
}

void LedgerImpl::InitializeDatabase(bool execute_create_script,
                                    ResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  ResultCallback finish_callback =
      std::bind(&LedgerImpl::OnInitialized, this, _1, std::move(callback));

  auto database_callback = std::bind(&LedgerImpl::OnDatabaseInitialized,
      this,
      _1,
      finish_callback);
  database()->Initialize(execute_create_script, database_callback);
}

void LedgerImpl::OnInitialized(type::Result result, ResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result == type::Result::LEDGER_OK) {
    StartServices();
  } else {
    BLOG(0, "Failed to initialize wallet " << result);
  }

  while (!ready_callbacks_.empty()) {
    auto callback = std::move(ready_callbacks_.front());
    ready_callbacks_.pop();
    callback();
  }

  ready_state_ = ReadyState::kReady;

  callback(result);
}

void LedgerImpl::OnDatabaseInitialized(type::Result result,
                                       ResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

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

void LedgerImpl::OnStateInitialized(type::Result result,
                                    ResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to initialize state");
    return;
  }

  callback(type::Result::LEDGER_OK);
}

void LedgerImpl::CreateWallet(ResultCallback callback) {
  WhenReady(
      [this, callback]() { wallet()->CreateWalletIfNecessary(callback); });
}

void LedgerImpl::OneTimeTip(const std::string& publisher_key,
                            double amount,
                            ResultCallback callback) {
  WhenReady([this, publisher_key, amount, callback]() {
    contribution()->OneTimeTip(publisher_key, amount, callback);
  });
}

void LedgerImpl::OnLoad(type::VisitDataPtr visit_data, uint64_t current_time) {
  if (!IsReady() || !visit_data || visit_data->domain.empty()) {
    return;
  }

  auto iter = current_pages_.find(visit_data->tab_id);
  if (iter != current_pages_.end() &&
      iter->second.domain == visit_data->domain) {
    return;
  }

  if (last_shown_tab_id_ == visit_data->tab_id) {
    last_tab_active_time_ = current_time;
  }

  current_pages_[visit_data->tab_id] = *visit_data;
}

void LedgerImpl::OnUnload(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady())
    return;

  OnHide(tab_id, current_time);
  auto iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void LedgerImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady())
    return;

  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void LedgerImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady())
    return;

  if (!state()->GetAutoContributeEnabled()) {
    return;
  }

  if (tab_id != last_shown_tab_id_ || last_tab_active_time_ == 0) {
    return;
  }

  auto iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end()) {
    return;
  }

  const std::string type = media()->GetLinkType(iter->second.tld, "", "");
  uint64_t duration = current_time - last_tab_active_time_;
  last_tab_active_time_ = 0;

  if (type == GITHUB_MEDIA_TYPE) {
    base::flat_map<std::string, std::string> parts;
    parts["duration"] = std::to_string(duration);
    media()->ProcessMedia(parts, type, iter->second.Clone());
    return;
  }

  publisher()->SaveVisit(iter->second.tld, iter->second, duration, true, 0,
                         [](type::Result, type::PublisherInfoPtr) {});
}

void LedgerImpl::OnForeground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady())
    return;

  if (last_shown_tab_id_ != tab_id) {
    return;
  }

  OnShow(tab_id, current_time);
}

void LedgerImpl::OnBackground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady())
    return;

  OnHide(tab_id, current_time);
}

void LedgerImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    type::VisitDataPtr visit_data) {
  if (!IsReady())
    return;

  std::string type = media()->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
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
  if (!IsReady())
    return;

  std::string type = media()->GetLinkType(url, first_party_url, referrer);

  if (type.empty()) {
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

void LedgerImpl::GetActivityInfoList(uint32_t start,
                                     uint32_t limit,
                                     type::ActivityInfoFilterPtr filter,
                                     PublisherInfoListCallback callback) {
  WhenReady([this, start, limit, filter = std::move(filter),
             callback]() mutable {
    database()->GetActivityInfoList(start, limit, std::move(filter), callback);
  });
}

void LedgerImpl::GetExcludedList(PublisherInfoListCallback callback) {
  WhenReady([this, callback]() { database()->GetExcludedList(callback); });
}

void LedgerImpl::SetPublisherMinVisitTime(int duration) {
  WhenReady(
      [this, duration]() { state()->SetPublisherMinVisitTime(duration); });
}

void LedgerImpl::SetPublisherMinVisits(int visits) {
  WhenReady([this, visits]() { state()->SetPublisherMinVisits(visits); });
}

void LedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  WhenReady([this, allow]() { state()->SetPublisherAllowNonVerified(allow); });
}

void LedgerImpl::SetPublisherAllowVideos(bool allow) {
  WhenReady([this, allow]() { state()->SetPublisherAllowVideos(allow); });
}

void LedgerImpl::SetAutoContributionAmount(double amount) {
  WhenReady([this, amount]() { state()->SetAutoContributionAmount(amount); });
}

void LedgerImpl::SetAutoContributeEnabled(bool enabled) {
  WhenReady([this, enabled]() { state()->SetAutoContributeEnabled(enabled); });
}

uint64_t LedgerImpl::GetReconcileStamp() {
  if (!IsReady())
    return 0;

  return state()->GetReconcileStamp();
}

int LedgerImpl::GetPublisherMinVisitTime() {
  if (!IsReady())
    return 0;

  return state()->GetPublisherMinVisitTime();
}

int LedgerImpl::GetPublisherMinVisits() {
  if (!IsReady())
    return 0;

  return state()->GetPublisherMinVisits();
}

bool LedgerImpl::GetPublisherAllowNonVerified() {
  if (!IsReady())
    return false;

  return state()->GetPublisherAllowNonVerified();
}

bool LedgerImpl::GetPublisherAllowVideos() {
  if (!IsReady())
    return false;

  return state()->GetPublisherAllowVideos();
}

double LedgerImpl::GetAutoContributionAmount() {
  if (!IsReady())
    return 0;

  return state()->GetAutoContributionAmount();
}

bool LedgerImpl::GetAutoContributeEnabled() {
  if (!IsReady())
    return false;

  return state()->GetAutoContributeEnabled();
}

void LedgerImpl::GetRewardsParameters(GetRewardsParametersCallback callback) {
  WhenReady([this, callback]() {
    auto params = state()->GetRewardsParameters();
    if (params->rate == 0.0) {
      // A rate of zero indicates that the rewards parameters have
      // not yet been successfully initialized from the server.
      BLOG(1, "Rewards parameters not set - fetching from server");
      api()->FetchParameters(callback);
      return;
    }

    callback(std::move(params));
  });
}

void LedgerImpl::FetchPromotions(FetchPromotionCallback callback) {
  WhenReady([this, callback]() { promotion()->Fetch(callback); });
}

void LedgerImpl::ClaimPromotion(const std::string& promotion_id,
                                const std::string& payload,
                                ClaimPromotionCallback callback) {
  WhenReady([this, promotion_id, payload, callback]() {
    promotion()->Claim(promotion_id, payload, callback);
  });
}

void LedgerImpl::AttestPromotion(const std::string& promotion_id,
                                 const std::string& solution,
                                 AttestPromotionCallback callback) {
  WhenReady([this, promotion_id, solution, callback]() {
    promotion()->Attest(promotion_id, solution, callback);
  });
}

void LedgerImpl::GetBalanceReport(type::ActivityMonth month,
                                  int year,
                                  GetBalanceReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    database()->GetBalanceReportInfo(month, year, callback);
  });
}

void LedgerImpl::GetAllBalanceReports(GetBalanceReportListCallback callback) {
  WhenReady([this, callback]() { database()->GetAllBalanceReports(callback); });
}

type::AutoContributePropertiesPtr LedgerImpl::GetAutoContributeProperties() {
  if (!IsReady())
    return nullptr;

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

void LedgerImpl::RecoverWallet(const std::string& pass_phrase,
                               ResultCallback callback) {
  WhenReady([this, pass_phrase, callback]() {
    wallet()->RecoverWallet(pass_phrase, callback);
  });
}

void LedgerImpl::SetPublisherExclude(const std::string& publisher_id,
                                     type::PublisherExclude exclude,
                                     ResultCallback callback) {
  WhenReady([this, publisher_id, exclude, callback]() {
    publisher()->SetPublisherExclude(publisher_id, exclude, callback);
  });
}

void LedgerImpl::RestorePublishers(ResultCallback callback) {
  WhenReady([this, callback]() { database()->RestorePublishers(callback); });
}

void LedgerImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    type::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  WhenReady([this, window_id, visit_data = std::move(visit_data),
             publisher_blob]() mutable {
    publisher()->GetPublisherActivityFromUrl(window_id, std::move(visit_data),
                                             publisher_blob);
  });
}

void LedgerImpl::GetPublisherBanner(const std::string& publisher_id,
                                    PublisherBannerCallback callback) {
  WhenReady([this, publisher_id, callback]() {
    publisher()->GetPublisherBanner(publisher_id, callback);
  });
}

void LedgerImpl::RemoveRecurringTip(const std::string& publisher_key,
                                    ResultCallback callback) {
  WhenReady([this, publisher_key, callback]() {
    database()->RemoveRecurringTip(publisher_key, callback);
  });
}

uint64_t LedgerImpl::GetCreationStamp() {
  if (!IsReady())
    return 0;

  return state()->GetCreationStamp();
}

void LedgerImpl::HasSufficientBalanceToReconcile(
    HasSufficientBalanceToReconcileCallback callback) {
  WhenReady(
      [this, callback]() { contribution()->HasSufficientBalance(callback); });
}

void LedgerImpl::GetRewardsInternalsInfo(
    RewardsInternalsInfoCallback callback) {
  WhenReady([this, callback]() {
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
      info->is_key_info_seed_valid = util::Security::GetPublicKeyFromSeed(
          secret_key, &public_key, &new_secret_key);
    }

    callback(std::move(info));
  });
}

void LedgerImpl::SaveRecurringTip(type::RecurringTipPtr info,
                                  ResultCallback callback) {
  WhenReady([this, info = std::move(info), callback]() mutable {
    database()->SaveRecurringTip(std::move(info), callback);
  });
}

void LedgerImpl::GetRecurringTips(PublisherInfoListCallback callback) {
  WhenReady([this, callback]() { contribution()->GetRecurringTips(callback); });
}

void LedgerImpl::GetOneTimeTips(PublisherInfoListCallback callback) {
  WhenReady([this, callback]() {
    database()->GetOneTimeTips(util::GetCurrentMonth(), util::GetCurrentYear(),
                               callback);
  });
}

void LedgerImpl::RefreshPublisher(const std::string& publisher_key,
                                  OnRefreshPublisherCallback callback) {
  WhenReady([this, publisher_key, callback]() {
    publisher()->RefreshPublisher(publisher_key, callback);
  });
}

void LedgerImpl::StartMonthlyContribution() {
  WhenReady([this]() { contribution()->StartMonthlyContribution(); });
}

void LedgerImpl::SaveMediaInfo(
    const std::string& type,
    const base::flat_map<std::string, std::string>& data,
    PublisherInfoCallback callback) {
  WhenReady([this, type, data, callback]() {
    media()->SaveMediaInfo(type, data, callback);
  });
}

void LedgerImpl::UpdateMediaDuration(uint64_t window_id,
                                     const std::string& publisher_key,
                                     uint64_t duration,
                                     bool first_visit) {
  WhenReady([this, window_id, publisher_key, duration, first_visit]() {
    publisher()->UpdateMediaDuration(window_id, publisher_key, duration,
                                     first_visit);
  });
}

void LedgerImpl::GetPublisherInfo(const std::string& publisher_key,
                                  PublisherInfoCallback callback) {
  WhenReady([this, publisher_key, callback]() {
    database()->GetPublisherInfo(publisher_key, callback);
  });
}

void LedgerImpl::GetPublisherPanelInfo(const std::string& publisher_key,
                                       PublisherInfoCallback callback) {
  WhenReady([this, publisher_key, callback]() {
    publisher()->GetPublisherPanelInfo(publisher_key, callback);
  });
}

void LedgerImpl::SavePublisherInfo(uint64_t window_id,
                                   type::PublisherInfoPtr publisher_info,
                                   ResultCallback callback) {
  WhenReady(
      [this, window_id, info = std::move(publisher_info), callback]() mutable {
        publisher()->SavePublisherInfo(window_id, std::move(info), callback);
      });
}

void LedgerImpl::SetInlineTippingPlatformEnabled(
    type::InlineTipsPlatforms platform,
    bool enabled) {
  WhenReady([this, platform, enabled]() {
    state()->SetInlineTippingPlatformEnabled(platform, enabled);
  });
}

bool LedgerImpl::GetInlineTippingPlatformEnabled(
    type::InlineTipsPlatforms platform) {
  if (!IsReady())
    return false;

  return state()->GetInlineTippingPlatformEnabled(platform);
}

std::string LedgerImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args) {
  if (!IsReady())
    return "";

  return publisher()->GetShareURL(args);
}

void LedgerImpl::GetPendingContributions(
    PendingContributionInfoListCallback callback) {
  WhenReady([this, callback]() {
    database()->GetPendingContributions(
        [this, callback](type::PendingContributionInfoList list) {
          // The publisher status field may be expired. Attempt to refresh
          // expired publisher status values before executing callback.
          publisher::RefreshPublisherStatus(this, std::move(list), callback);
        });
  });
}

void LedgerImpl::RemovePendingContribution(uint64_t id,
                                           ResultCallback callback) {
  WhenReady([this, id, callback]() {
    database()->RemovePendingContribution(id, callback);
  });
}

void LedgerImpl::RemoveAllPendingContributions(ResultCallback callback) {
  WhenReady([this, callback]() {
    database()->RemoveAllPendingContributions(callback);
  });
}

void LedgerImpl::GetPendingContributionsTotal(
    PendingContributionsTotalCallback callback) {
  WhenReady([this, callback]() {
    database()->GetPendingContributionsTotal(callback);
  });
}

void LedgerImpl::FetchBalance(FetchBalanceCallback callback) {
  WhenReady([this, callback]() { wallet()->FetchBalance(callback); });
}

void LedgerImpl::GetExternalWallet(const std::string& wallet_type,
                                   ExternalWalletCallback callback) {
  WhenReady([this, wallet_type, callback]() {
    if (wallet_type == "") {
      callback(type::Result::LEDGER_OK, nullptr);
      return;
    }

    if (wallet_type == constant::kWalletUphold) {
      uphold()->GenerateWallet([this, callback](type::Result result) {
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
      bitflyer()->GenerateWallet([this, callback](type::Result result) {
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

    if (wallet_type == constant::kWalletGemini) {
      gemini()->GenerateWallet([this, callback](type::Result result) {
        if (result != type::Result::LEDGER_OK &&
            result != type::Result::CONTINUE) {
          callback(result, nullptr);
          return;
        }

        auto wallet = gemini()->GetWallet();
        callback(type::Result::LEDGER_OK, std::move(wallet));
      });
      return;
    }

    NOTREACHED();
    callback(type::Result::LEDGER_OK, nullptr);
  });
}

void LedgerImpl::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ExternalWalletAuthorizationCallback callback) {
  WhenReady([this, wallet_type, args, callback]() {
    wallet()->ExternalWalletAuthorization(wallet_type, args, callback);
  });
}

void LedgerImpl::DisconnectWallet(const std::string& wallet_type,
                                  ResultCallback callback) {
  WhenReady([this, wallet_type, callback]() {
    wallet()->DisconnectWallet(wallet_type, callback);
  });
}

void LedgerImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  WhenReady([this, callback]() { database()->GetAllPromotions(callback); });
}

void LedgerImpl::GetAnonWalletStatus(ResultCallback callback) {
  WhenReady([this, callback]() { wallet()->GetAnonWalletStatus(callback); });
}

void LedgerImpl::GetTransactionReport(type::ActivityMonth month,
                                      int year,
                                      GetTransactionReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    database()->GetTransactionReport(month, year, callback);
  });
}

void LedgerImpl::GetContributionReport(type::ActivityMonth month,
                                       int year,
                                       GetContributionReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    database()->GetContributionReport(month, year, callback);
  });
}

void LedgerImpl::GetAllContributions(ContributionInfoListCallback callback) {
  WhenReady([this, callback]() { database()->GetAllContributions(callback); });
}

void LedgerImpl::SavePublisherInfoForTip(type::PublisherInfoPtr info,
                                         ResultCallback callback) {
  WhenReady([this, info = std::move(info), callback]() mutable {
    database()->SavePublisherInfo(std::move(info), callback);
  });
}

void LedgerImpl::GetMonthlyReport(type::ActivityMonth month,
                                  int year,
                                  GetMonthlyReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    report()->GetMonthly(month, year, callback);
  });
}

void LedgerImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  WhenReady([this, callback]() { report()->GetAllMonthlyIds(callback); });
}

void LedgerImpl::ProcessSKU(const std::vector<type::SKUOrderItem>& items,
                            const std::string& wallet_type,
                            SKUOrderCallback callback) {
  WhenReady([this, items, wallet_type, callback]() {
    sku()->Process(items, wallet_type, callback);
  });
}

void LedgerImpl::Shutdown(ResultCallback callback) {
  if (!IsReady()) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  ready_state_ = ReadyState::kShuttingDown;
  ledger_client_->ClearAllNotifications();

  wallet()->DisconnectAllWallets([this, callback](type::Result result) {
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

void LedgerImpl::OnAllDone(type::Result result, ResultCallback callback) {
  database()->Close(callback);
}

void LedgerImpl::GetEventLogs(GetEventLogsCallback callback) {
  WhenReady([this, callback]() { database()->GetLastEventLogs(callback); });
}

void LedgerImpl::RestoreVGs(RestoreVGsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    backup_restore()->RestoreVGs(
        R"(
{
    "backed_up_at": 1640598940,
    "vg_bodies": [
        {
            "batch_proof": "GFqAbrQnCx2FV6QUwV02xwrpX0fdY0qq/xnWlrD/UAteg0yM3CuLp4duCpNEBAc7Vq450cfMQbIB0ghAIsb7Cw==",
            "blinded_creds": "[\"TApFMMyUppeWJnCRvRCFxQPG52jIiVGjY41GXNKzqHI=\",\"jl4fJQySgV68LO2yBeA1De5s9QSiSgpXAkOTMLuACiw=\",\"bgt5DhoRWOcdf+CC7o7VCrWSarcFvY7R+rZZVB+xxTw=\",\"7lL+98lI//nyNDmVTyM0cpyIG3bfw/2b5kjaJu3U3xY=\",\"2Cf7kfzRtDjsw+tQBahrOuWCuXtNpsmmJsS6Tg6Cvj4=\",\"qFkEV98eTntgiW3jR56CjA/1Mkbyd0XkAL5inuh5Q1U=\",\"5ozJNXgQ22KKwiuLUC8EKBF5xHQlHEtXgnlSmIZBrks=\",\"uO9I3Yt3FRlo7r9LAZ5FSYFlab0+czlQBRblVJSCsl0=\",\"3lvmNwRbeZCyegz0jXPpONZNTmLzUqRDT8xcSsEZcTg=\",\"iDIqVq7QMhzBT1aZNpP7YAHdEhLMUv12DVsORC9zom4=\",\"ijeasO5l5Hgb9kPZTnwUGzRyalngKd2IFDZ4Kk42KwM=\",\"orX6Rq14keFrC/Le0/sAk2aEqSB2NeB/B2P2o13N7BY=\",\"Ag+u0dVA/+Yf3xo3tdYBLF55l1LX7E77Fy9Urj0+RFk=\",\"SIVv5oe/+Ltjuk71esUPOyjwEMQw6/f2BbIu1Zt8Ygk=\",\"rNH6NVCNsENMzxkz/bxdTsZvJyII3/PpSsW4zKq7fjw=\",\"+mPOCl0k3FKcWuB+4WDRHolgD8VhCPCkV3LqurGDOEo=\",\"IHmcnqwyn7QKrVr1eiJvuLCtn5FE3dc+lwr8P93AICs=\",\"Ar6bnVIayOhLoPwxEMFDgByB95BWfBjVKBhOLCp+hXw=\",\"KpBDPewclD5u2YDLhTY04gslDf2GZqy6xY710FQ6rFg=\",\"jpbCBm+F8EcOTqLGgHIv0T8SSlvsZys4gadeNDlbTkQ=\",\"MlP8GnrNpo0Q6pgMbvvp6+5q1pMbnzsi7OriouxnMyA=\",\"AF0Vd7mz2jhLNTYRVReeugpciFGYgmV36R6gKhHk+CU=\",\"gJTCKXRyQ2efhfKVEf58JRTu9VOvzcBM4YPIoaWEaCo=\",\"pMzqBiBftOTw82pRYSZaIoIeiuvp9VazyLc2jF4vTBo=\",\"MJkTd3w2KgdDfRr+0MtwJz2heD5oecjDQO6QMAPElAg=\",\"Dmt7xLtyYNH7MwF1n8rYW9OQSR/3QQRCSlvzcbY0PWs=\",\"Knu22iceMzUO3lneow/e8q+ugjn331AWCdrY1KYORX4=\",\"usUFXm0crWz84grQSqfUw/5X2KbWB2AZf1M5gcGwkxM=\",\"0sZwa0HxTNj4ZzAPJKM3L+mW8XA3PdxjrUvTk8zGzUU=\",\"PHkSmaBNDuHpmLb662WO/7naH5If0bavqyGhL8QspXo=\",\"llJm8uFk8NB5VpgW70M39oJBTBL8Shh4ZQXbGirOI1o=\",\"2OLUx1c+ICzifkKohKAhRJdogkCmo3dF40Zwyzh0Xy8=\",\"7gPn36q3D3zCaM0TGn2UcFDabjivCt4vRvaeLr6uBkY=\",\"hvjPN0jIqTQJxOqm+OA9KWtSkHw7c1GUhpkzXDJDr0c=\",\"4kMPCaUpEnfNd9S5MmH+7Fymx1S2WbIkpkH9llSF/zM=\",\"tooP+2XntVuEm7WgIVFSOf8ipu2bXcLB8TC6UGsptiE=\",\"Bh/cImZGuldzX4VQlAtVUTxsFdbDTLjcUqtLM/jNtgs=\",\"pBuICFwYnkJcN58Ez/bHR0RMiAKtPinx9mjIvj7F1B0=\",\"2tm3HoKwN7BGOFXmj4tW07FqgvqXvvEZUMUrBF+YdzU=\",\"JLel4q8j1v/Ho1FShskfN2vkZelbQvZ7eGQopoq8o3A=\",\"xKrS3kyDM0Whygfk584JTZxw7i+M2mbSYf4+veJ9OgE=\",\"fL707Kpa4xUcyM3v64PyakoeS+QNZRbraXhJt+oG8Hk=\",\"QAmLl35PsNYhQ+hkbYr7E7JIo2h/nMUFCtFTVbSTBDY=\",\"wo4oyPqHGF8ykn50SObkTesuP/7rXdog2LQrli5KbUU=\",\"6CpSjYKpkj1aPraytwG5/SXDHoUxo4HZ3m909y8sVxI=\",\"8PZ+dq+hxN3WApIL4zFlU478VcmRPvEfgxHEQnM3YHI=\",\"ci6H8kVBRqofDxrp5GEGozJelcjskz6+iv375hJOjms=\",\"khUfOZ7Y3/sXlDXhqTzOEidlzf14+Zb/O760/SN48AQ=\",\"MEERmq7d3liUQdO3RpLkRqlapaoOAOrSWDpzuEuoWy8=\",\"du/CfpC0HP95AKdgaXJAfxTwkLbqtW06GQLgQZ22Vyw=\",\"PCroyjSfCtl1ynRAwvOIRowcLveGSTGXAEqAaaNynX4=\",\"enL8xp5d5MN40e/P0oQtIDBGdWH5OogHaCQFCOsoy3o=\",\"ODSxuW5V/pLwUotPeMtOTvkNhiR5KY/gdykIqHKO4hk=\",\"sGsHdqtge/2RoZN7BaJ6vVRrX8FoK8yuvJDRr2lBunw=\",\"UrvwxxR6Rs4qGS6W+uYoMK2x7elABzEnGY1pMqDNukk=\",\"blzllKBC8xYisJk61GbQb2sJDZsS2PSHNcHW0dGf/Rs=\",\"7DF9znzKBqPf/Wo5Z9QFdJfv6Pm7HZNjy8wE5dI7VGQ=\",\"2M7HMpwRpNFogAmXXImzzBuaQLy6+5QSJMwLLeFrQVg=\",\"nOwcgGHE9ZpDm4VexfiPvj469s5VoooDsoxwnsYPzk0=\",\"QgRemcPCzMGrAmQ2r3b8/2IGGcLx09TrwthNFJqBhCg=\",\"fteGSwsJx/Tjix6K60Y2YSV1EOLftiPu+6SYqjA+6GY=\",\"Diu9If0Aa4bFBTuX24+EgVJGVHPytMizfGzStY3sOBk=\",\"xs8R0yOsNUiKBnWXnmP7eCpbnJNP0fuL7ZiagpxfpnE=\",\"IPy/Cq0NlijNZQg05KrF6kURSbN37eiaJejqUoLZ238=\",\"kr3V/6l+2NIbyPMwyZmm98xoX3cia+kSP4mJ4L2wlDo=\",\"qJDbHsgFcK0rZAYDBkHHp8bz6G1F+2zwAj23xRg68Wk=\",\"hiTTVgygecngFeawlJ2ioE0aDKiBjfNocPBOBai86E4=\",\"FP2Fgya5IelLPYKOfMmbuCwq/gRSuEdCVYk0XDGCLwQ=\",\"VLOMFjiev6LyKa5pEVqM7hiLN/IwBpfvkTDH3CgXEBQ=\",\"En79wzGLfOUCNk9EmC/BwnTx9SKqo/z94QUxB+jr81w=\",\"yJT4mTGihxufi1RPQaysguUiw6mKqyrVTjra3Nryj2g=\",\"NpKjR4ynBXAuoCWOP8lEBK/pctPZEEXNxmun9V8rVHQ=\",\"BIWLii8GCbMseO4h1jjUEB/mCdQWP4+74ICBgU+TW1Y=\",\"Qlg0k+Hf5aM4J8rGu5PSGhYF5M6NayM1vKoFwdOJ4Ak=\",\"7hel8VrF9exRwJaC9MkJb8kK+qtLpXH+fCpfqfSypyk=\",\"PMjRy+/WKr8H9CsQGbDCWUgxitsBJjiBNHFLL/hodiM=\",\"rEPCaplKBpD5jjYJmYRD10vyMSLmTzp/vejayBLtdXg=\",\"Dv5kiLbiWaVGrm4A3o9nEIFEvXXG6MpA6uLyFkaPI0Q=\",\"Zmv3dKOvjsUovDHlgrr57O1Bqa7n0xK0F/LOflPu7nI=\",\"mq4SfDIG8G97nXO5yivuW9fiYOr8ZzJWBs6HDFfgF1c=\",\"0kgtraIa4rAeC2dPq+y4rcDILePC9VbFdjssj/gMx38=\",\"KDyvUWuhbJ++asTGPa2C+p9ikHX1mKkbLQW6muPDhDE=\",\"6pdtV/n2h8VfobpsENRzAyyvxc6Y1n6H72jw8nYDKWc=\",\"ujROMmnVJthc7VSLJDYZCTCLaDBF95E8erPlzLrqPXY=\",\"kgVFGlpDwKUrYdnNsekYyfvhmrf0zykdpU68Gck/wkk=\",\"HGDtkcDeUuys3Yef9CbovR3k/xfNgf5INMgGzQQNshE=\",\"ajTSrW4M9RBCV/u2ZvRUjJC/STT1hX8L1gh6mRNouDc=\",\"2tiqr2WONlJWZec//TimBcXgwr7pkRA/lYr5KlCV9zE=\",\"5vEH6eDNcWmrHIsgKEK2wEC/pBqEZ2GSSYvn5NqpbyU=\",\"yDWpZzIcUb56oAPlvwQkconiMWST1VUrUQmetvR6mTk=\",\"VF6sgPlXpZMQHHr6QMv+UFsz+ZslMLgD/JrwWqSxIXM=\",\"gioN496O1/yf3GvB5KipJa/jpK5doCZVL7QErM0lo1Q=\",\"vDZmDvtEJifqZ5WAuDtnVZ/xtRwb44ITtrQ+lfVhYD8=\",\"fo75X6dTjENDSw1F8nqTF2ssfRBMaOslSphr/86IEXk=\",\"8qZsdiAVCq2nGE3JzTZ+/yFBTtAPo0bgD6brjsYn3Bc=\",\"BlyvV48Lwx//YdCpnP+HRQk/1E39Ygz8NCEMSTjrqV0=\",\"hnKAIaOmoDUoyyOsE/H1DDWHyY0NtaB86fW7sMaqQ1w=\",\"SJZbua27xW7INjVJOUf9SIXUpteqgoMQUZxZRxgn4mY=\",\"Stw/wBawnILrB2geXv9S9WXijcOtdtWLxjxjJpuqvHg=\",\"avsdEizctzI2YnW8CZxTDqMCiBE4wW7SZcUvNRu4DAo=\",\"KNYYpjEKRwDNhFCGkgSsMcaNiakiX5vdLamS7oqGelw=\",\"mlVFbzTXBxBH5cRKol0W6Ze81Ve5m8oY0aGTxCqvWAc=\",\"TrpGShTSJmnetZjwS0KNSNf/nZ7PcQORLT6mpPcLeQw=\",\"ZnMWHyewVZQCl5tC84okrvUd8QnazeWJ2ZXxCn4jSFs=\",\"nulaqyS0b2p5tjZrLUSD0ZHGBJOcqTkFXOnDLfhCcSs=\",\"KGdUnvG7VGwY7FNah1MbFnoTLWPTl615bcr5nCrZ22U=\",\"nERVbf0vOlp3ffn/kj9puYq/vdRyEPqx7f4fJRYWYyQ=\",\"6nhDFmBpxEcrvT5+j0Ivy1ooU741TUcmxkjkuIKjqRQ=\",\"ejVngWCi7MzuJdJ+ZHtgzNMLc5tDuiBZ0A5zSgOu3zk=\",\"Dgr352QTDRtZi8mago1Gie83ZmCaZFojxt1GzOhjXHE=\",\"FvRvMZfRA3X3zqlR7AAfX6zFU/s4JzLsGoR0+wYI2QM=\",\"lj44UMQtK8aGbYPx11BFKrouKTpQY+N6hD8wMnmej2E=\",\"Kn6RMGvOae8k69Q4cJnrL143u5y/ZHG+srho6nmDVF8=\",\"kASCShDkx0DXcg1Prd8uNVhhbqvyU0ZHsNWeh20czwg=\",\"HGnmklJa0f9+2MVsxIXM+oyQ3HMOCxOCquWX0lqgJCc=\",\"IMlEdW4rfbJ1a9hP4izUwt5OISpskW22Ob651ZhUhic=\",\"PBwdfWgYJ1DN94TMzWHk5u87X043rwWSUf8BAzPSQH8=\",\"1qvGyoCyQUpIGe5dWQvGT6Xmlk6bUiUF4g8DceQPZHI=\",\"ylxlN1L85rHqwh8EB4PHpC+VgEhHTiHrnJAQhKKBb3s=\",\"vFNyxMF8bc4HLbz/j9rBplD+gdTBDTwQj+6DWVkKSjc=\"]",
            "creds": "[\"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\"]",
            "creds_id": "99b4f6f8-11f3-46a7-9cfa-8f119f242815",
            "public_key": "6AphTvx13IgxVRG1nljV2ql1Y7yGUol6yrVMhEP85wI=",
            "signed_creds": "[\"YrkCpEWh+aSiBZht8aidocq30u4WtRTFJcgNs+qqxn8=\",\"4NyXItdxfXbtH3JR23rsxZry11AoNHd5WZBJhhXIPVs=\",\"cGFwLLeyupGzj/vjBsxyStj1y/XnCsxBL91XH3RTBQ8=\",\"+M/wqkNByzq1cLSHvGRE5ZYsyPxJRFCqz1jXvPSkcQs=\",\"pA3pHjoHtB2e9kNPOKqNiBjgyjo/19dcOwFNBFp5B1A=\",\"lEF1CmMjgFdwnLJJw2BXAvarI8iJ0s0h72n82sYrR2c=\",\"RCJFm45IqZjtqgY6Ii52qQcKjcrE9X78wiYT3dU84jo=\",\"CrG2xwKvp1ogg0fEM64lasz31epQ9rLQXWVsdhlJaEo=\",\"guUCoIB+Zs5lpVTwRE9gF8JrTWfPZaBaj3HjhxEIPR0=\",\"gnY0GRKRfjHBtTXIo0r0COdtkaFXr/drwDxF+hJnjQw=\",\"ygGMnQ3b9EuaMdHOGX8CIgGNtT9qywuvAbNPnbij0Rs=\",\"LjfXVBkLu7LLZT5OUrWx8OoZZQNGsdWSNmh0tl03nWM=\",\"kqwpA/iXpSZfRdh0ssUlNaXFmcJ5AVIhf6NqLMxsZEc=\",\"1nNRpDjnKjrb++BgxHc9AOX3EGQcxjsV/xXmjz/IMQA=\",\"RnHgQYJlkHaKnxAjQcGsfAd9H935N+N+OMyukS9VsFs=\",\"WDhe9A8603rMYmB7SfY/tHXkjBto4L9fYN+qv2XPzy4=\",\"qEzheAJfRf2fFi4FI2rMEE1do/16BZuBAHNd6MXSvyE=\",\"GI7M1tr/0ORf+pTrfmS4Uge3tU6Kak3vv+Thxw/GtTA=\",\"kFuB+z8nAbariToxZ4IDGYiEsgCKnRH9q8baYwTeFXA=\",\"JgI5ZP4MUzPVg2MS/Sgcrp1kag9WNSdjP6medpXH/Fo=\",\"sBpGmCY/1i622Ka+Ui8jYTmwkhgXt2NeCyolyZ24Tjs=\",\"Yt/Xsz4MVUU2n3LCMdaq1gBspi9eHAP5EGuOEU/2Bko=\",\"BuSzbLY8ZiAhUlawKFtbrLAxBiAksf4AklCto+4XXjQ=\",\"SNpBvaIZ6LZCOiJGnfFSG15tYAfcSkyLGf/Sv0tNElw=\",\"rHa55u9uFVr/XKf7WC5f53jJV+HLAZZnpf/jHlgHMnI=\",\"ElDjKcuaadHc2G4JlwshRzKRbi2F8oAKuJigNDG/Bhk=\",\"zn6izqHUrhq3IrhjYkKsy+aHXPIFvTOwynji07tvgDY=\",\"QBJv4FHllKC9Fj5YXO+jUuaQ7TAGvHeaRIl9dw3hxyM=\",\"phG4dHnGae4Dgv6+0PvlKi0hc23Yof29arZ3WP840FE=\",\"HKZenFEL0Pf6153JXE9gg+uPzIZXeYb14Ypp31RbawM=\",\"EE6zhrnRwVpTplWCWFfKgwsgwrw44XR5b8x9zT3ZIyw=\",\"ePnT3peJhCwCvEiTsPBJC4+HnJsFXz3oVgkqvckYhx4=\",\"2jxY0JCMMS7LnpnfjJFhZagY9GVhbOpN1nZPBVh5XUc=\",\"ipgfPOu8/nCaajEOJxsgQsO009MHzPcJUHncTHoIjUs=\",\"MHxPGC2Wqpg22a8tTr/W8gm9Npf/Z90qMzZuQ74AiE8=\",\"Mis7L0u+FOLjhNloOFWPzfo0r7HkMyRwx0ZLWQ6Uwkg=\",\"0mh8HsIzUaNXlWG3LK7c3ja7YJoWh6QdqUzBJscZOyA=\",\"RLU4FDgr1/b2ttxZ8MNEX4PQHEAgzqeFmoKf+nLdeDA=\",\"kNP1pxZ7oenObHuayT5dh9PC1VXT5Bf07E+du8qddBA=\",\"xrkTq+S3LniML/PKpstPf3fX6A1XTWKscgCiJewLeHM=\",\"9mELATzhOgf8f779Io1m1frzdhlqIn+2WorJFkHr8Ao=\",\"HNtw4MjyKaP+Cj0PlnmOKimcmqlwfhdzxPPTFzez/lU=\",\"0pist4d7/hvJYEG0geiNLa1aU9g1319NhRuYf5NnsRQ=\",\"8OayTpcNp+o7/4PScJvPiYMl5mClMqaxtyMKhTEKI14=\",\"tEPA3wsXcGaNdnVkisddRYRe0W11sg0S1n/8LZeszCE=\",\"uN7VGrR4ocT6XCtoiP8SKWUhLDRcR/mGIeE8ClDcS0E=\",\"SiYFxY3slecg49XJ1a8AQPc+gIxSr7Bw785W0/c802g=\",\"GoJ4jVvi5YMLT6mc1BPIarNvuiub/HI91k0Jjk5BYhk=\",\"mM0FOYqHRZ/O77D+gUJngXa3xFBfcAKt5G3yFKPn2RU=\",\"oP4Ycp8BWD+u+sSz3SC2Wzxdvn4kjZ4G76jcD474uwI=\",\"0hlbxUKDuzyCn1hahSfSF0wxaIbIYXqTAMp27lNaLgQ=\",\"iCmLQBqMozwgS+AUPkjD99ZCutFb8GqyXc2G9xYpzj8=\",\"8kN9epAWqW2irNYbDVscidaOFZgZgFGgTN1jbtj+E38=\",\"ppDkRYOHa+X5Ik+gb2XXMDFPt8GtuXEEqczflPc+7Hs=\",\"Ws9owzW/TjiGY4Hx0sj/OGNL6y1kwbFoJJvO5g+J6zc=\",\"cIHF+rFk+ZuvNwXt5O284+bQN4Kxp+hF7HlQaAjpiAQ=\",\"nt347+XiL43jNbmHvwCU5sudSSuE9FDOoiC6Zk87vXo=\",\"ALh41Nc+uYIlujBiZLa84aTp5BK3xsU408I9XnxhfiQ=\",\"bpEcsQhN4fZhtAcVWezh0YEQkbmd6C7jlmur2zjghyM=\",\"lvJhlQqQpv/Fwcv0nlVqap8l0YCPD1z7FMhCGbPFc1w=\",\"3pbJ2woQu55cu4VQh+yOem+VlopaLCCToizvczeU0lQ=\",\"QHKCDpswKaViMTynfQl564ngRgLX2/ERrsuIy3OQ/lY=\",\"MMWV1/UhZLybFGK+SE2rPq9EPyEKF7IXLkwTdgyuJzI=\",\"jCTrov7/KddX1ah5hzDf5+22UJdGoaE1wAr4Jb8kUiY=\",\"NDmmuxS3ZMloDzduuEt8cWLymcewWR/VX890rWb2+Hs=\",\"wNJb9GLsI8pFcMkg1rCfJoAXHB/oojaNjFfgeWSJmzY=\",\"KibfgT3BLY35HGlN/ntT/0acs/CRmc4l0plszjgjqk0=\",\"xC0HDVzpcwgWJVLb3uC43lX+Eaeso4S9xJYNZN0RURk=\",\"CDElftDrBZt+HQg6ll0BaPw593xo0CrCqRZDsmnFjAE=\",\"Qj4ctWnpcU0C5pcG+wwexStVqSpXp/0aT7JpONIV414=\",\"6HL3edT+pUQS1FCDBk6D0Rf271yQzTeO8PFnKavw32o=\",\"aC6CvFgnHKkgik0z8h4PMa5YVPA0CxLMsyGRU0xGs30=\",\"MitBn6w4Y9xvxfytwoZbCl9rFZPiVQ9QKidYrgSriEo=\",\"pmTdPS0OJMuuSTvwpco2gQvnXsnyYsfZg7zBS41TnBw=\",\"Bu53pwZB4JpiyBVj237+r8PQunCt+GkftwXdjsTDzFI=\",\"UEt18OfxCXKfoHwyHWclt7Rdql2URvvxiq3rV3+zins=\",\"AFM4kQP6zVQ1+kQDzwCikvfIHE0IIoHNAsme+UpIQR0=\",\"2gTsRVuLzxMxKRdZ8Ox/25hUoYn1ihpOrd1YOMtckS4=\",\"asQ4XRB9sdmODSdPwtI5aIVnfoDehUdEIICGM/bVbjQ=\",\"voXARGZY4o+HU27N66tSYwqNmyWjlHPtt6ekRR1M5kQ=\",\"/t0he5tgCxSDb5MisVP94lN3n8MCg1npLAhbqetnwmo=\",\"ArbbIwqDnbyGBEwve5b7ON2ftRKWnQNQGJ2To4817mo=\",\"QN1JUJ2077nTT2AFbmOLwwHbzcIYiRk9nUrUv6Pt6X8=\",\"ThV5vVytnft6A6ctDGNW6u5ktiXfAsxXDD4pdT/RY2E=\",\"Zl4YAG60/dd+8dJPmOwhEoc7BtpJwMDRl+3T56LEQg4=\",\"iHGeJ3LYv8uD7RrwbMXY19FVaUyEUYmVlz0ckOYNXic=\",\"xGVNiWzjkKz/pY6TYvsiasphZnvjuXhlNtRvRb69LDc=\",\"9p1mObeib1shGeA7KBwBFXKSVkxAqWg/Volo+2zUcwI=\",\"ZtxKElBF2mr51xcX4HPZb0PuYuA6CXETNl7+hqalpAk=\",\"bnknYm7g8LMZYV9rozdIPn8Ac6Iu6GMwsYzn5rUoXUY=\",\"dPSgwFIFOIrw0nBmhHd0cNe0y7UroRqOTIrxlcc7KAU=\",\"8FtdVDWNEUWfk0Tw/14c6a60s8tnIqqkT3e8eoPygH0=\",\"lItOx8is3+KOZ8Swlhz7AIYpaTcgDd4dmY/jomauyxg=\",\"mG4hVB5YMCI21owiV2rSGowSu6Zh3Ni7SxBCTaPwty8=\",\"6o8TiwG2xKtscgvPfxQmm2ELoW3NN/VjiUAx5f36aXY=\",\"jvtMHbrQ3END2vhOmyIxAmVGLAwEF6zp/8+zCQsVJXM=\",\"9CJGtudl7dK8Uuxf7vXBFmzg3nwndI86aQnxNQZ/sCk=\",\"4J4qSwOQ4DeDHdyFZ6j6bmMgJf30K5MWfGY0LmNkyRM=\",\"zO5zeCKrGRSJ5YioxPaW84ZXWtlcmHF8zpubXgv3iG8=\",\"hAeCJqf7DAgNFM53E+jAgTON6GS8m02e7o603DpVals=\",\"0lkvmyhw+4Wwtw8CjP1GpqgByH9kGue2xb19Q6O4Gmc=\",\"QNgu/7axSRtfrMMILgpuwF57G0207+EzRAkFwTqShgs=\",\"xromP0u8ksjdw3aGDZ2f54rGthzpQKixkAqDqlqsaB0=\",\"fG1dXQxuW7QtmhNwpVonC4gnLT99Weqt3x0ZbLQm3D0=\",\"Nl2KJAXtcrZRp8bQRkE3SZYKDt2y5qAvaIhs641ELnU=\",\"hhBFQrLmjPq/lxkDqC+taGISt0yj/AeY6owNcq+P/QU=\",\"tmUj5odtN36gahCVgGz0lF9J/QbuBnjBpGlIo2Q/smA=\",\"ItOunhVVEXewiulJRRf+Ly+EPQk96Mb7l7UgPUvfxXY=\",\"HFAMvqafV6gRMWwz0eDrH3h6mm5+m6qOlP9JRBUxWD8=\",\"+L0vWcV5ljuFhCUAlvBRy6Yw46igeiwaVa9bNZaFW3E=\",\"OoT4ksheuYxcFs7deNqjYdRlV6H1a/tPPZZWhoFnhlA=\",\"knQHaWDwUTDNHSHp9O1LRyIkMhSpgd+xXUxqCgINYjM=\",\"MFga4lJsGUJjTMQmh51FvcC3ARZwpTyw4LTXmHMRvCY=\",\"iu2t8srFIVWQHLMgoun6jHh1fHhtCTUuwK/ZGQIocxY=\",\"4M+q0gkeT8SMNAsNYKkRJu99eWp/9Z7yLtWDpdAW+UY=\",\"7CmQP4JLCDkKu7SCCQaaSkNjTaq3QdsG1FkeQktWhm8=\",\"ULDSb/ZJGidkgMC4m+hl2byCPmOkjDANYYJyiu3EA38=\",\"bq+88Hsbo3UZZib5R0uTs/0zkSF6zv2ikavUOZiuDUY=\",\"Qv7ji5RBAPS7YoKb2W/If3zwGxKy2jq3Qd+9W089pwg=\",\"BrESMxbDF4sq2kuvDwffsDI7aJ4I1gkXFB8wB/RDLkY=\"]",
            "status": 4,
            "tokens": [
                {
                    "expires_at": 1645736183,
                    "token_id": 1,
                    "token_value": "wruwH6yctVhDjBoEhbq0g+8aUeiFBRLPmF1enpKVRJpB/4toOBgFjz1NzTHsPUbr4sAx8TK7o/s+rrPgMc4ebhTEcbGtg2/a8qPNSpexhAo+o+f7ZOlkSw6DR9E+oeVa",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 2,
                    "token_value": "eNm+Fv4zgKoxCj/pDtpoEKBjChvhqo3tHmuezZcFPH8iI2LM0F0GHo5aKrd3kCDbogXhOs9VJHehtTMCUBsOEjSZ+WCDN6yLdfZGoecCjGyWCncYhUG0RXIpKdOcKoUg",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 3,
                    "token_value": "udpeKvKNHOK4HZ1z08jnO4tmnHfqNdHQ8L/qo+US0v8KBOhSK55JE6u2Ucdkb1xI6O3DNJwfbnPD7vHcX6CA41gSM1VJbsh8f9Cvcr51loXQDVNny680p0RO44Dj5RMw",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 4,
                    "token_value": "Lxh/2dgHc+a0gKZ32v4J2sJvQkTWXOQ8iGnX0KZjzwDZbSebSHFOKyK8mg7syAY+J374Sg2vVpfJYdcrIaPcReI74gnKCFd4L04zX3GBXbCupRe0RNWfGuvtU2l5elBZ",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 5,
                    "token_value": "WSmXp0XhZY3d6ZH+n2QPEjippIKbRhwbGssF4BXCB9sv6OODizBTtNbkMa0vWswg40EYjsLMqbjFI0pMCQlZ/EDRNcLNaAtXM+uCjBohSokNMWiKIFLM1NgK+0gTlYcR",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 6,
                    "token_value": "mcHFaVjBVZQs9p4DRc5bXLgILSf0yfBmGNFJu4y99CeYOfQc0Me+1RoovUk0N3ToELQEL3Le/pPehNaQv6fD+RDpnwuNGulsqY3ANc9JC/9l86HbgP2SS/9krqU6QBBN",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 7,
                    "token_value": "EgepGhK0V9f9lnNBgim235AlZpUvWxiFSl7QxCJECPYSSYzTSJegcQkY1u1ASUBhVR44egSOqNXCRcq+9Ao5yFYPT/YmE2p1p5kPFgrrdwVLD26tVCefT3bt/YSdA9Zl",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 8,
                    "token_value": "bfa0EFILOqKKCD7XJGNYBUB632CMxiU0eNppTaS8kalH6GwwJEmWDl8AzOlRHJab6VSMlzQvscqZPzxchFOS1SqEO0nA3VLlDPsF7383U80bYZhfFtFugLw6JXayqQpv",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 9,
                    "token_value": "SaNLuVLAlA3zvDgUchjZQ0DFC1tcwnsGXeXYpZBdhHvSVXaWq1TXKzJrTUWNBnQeAEcWQeAVDwPCjMUsoUeXOIpbO1uK4vTkiaqm7SiGBF/wZDMQC8BFIlQg7xyacwIV",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 10,
                    "token_value": "xQQDSUHvQDwZZRSu/agtzjRH4mvnbFj04QwU+eXLLFrUDsFMdb7rLUV0lGISX8MLG/R5ohuGBrqXWNcy2tlFoEbM1ZBCIfHUZ9mMj7R+gfW+2/5+om0SfHYW4ZKuhNtI",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 11,
                    "token_value": "0MjG1oDyhumlCeb1QPUfVjcLdvkOVcUyNmmK+PCqphlLA+0Jrz4hln1yoTXKuqhhi6CgpZFLeKclEuokAdJFab4e0D1kC5hmRsP3NmDeE+I7rPURkruPD+8Gwm9cHSdK",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 12,
                    "token_value": "YU2WHK1kXNuCIdtpwmmk/FBGZ1s+U/wI8tRzq+N8nX8EtlHYYR9iMCuS5BhCDteHwiy6B7EXrP9H6EYrc0K4CVKZewFtHMBT4U6lmYalQO1jg8kJUNjeHEQMuwpRrZM2",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 13,
                    "token_value": "E6zUvTAxUhSghiOdUOY0UEPKvhPp4XKEgxs0LLBZNO2LoT8QMqpC72ETWs8lHffCGkdbhXz2+QPyXb707ToJCbZZ6n091BUei1kQ/XAeA1yL3b7RekmOWhZ/8qg7wGJ7",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 14,
                    "token_value": "abvttHXtZaDiNlpY2zsYj8BGW7UFmoZj+l4nN5FQbjK1j+PYLUhtPxxfGHgAD/BZpJqto+aGYWho8x8MxKtv50YTPKQlkHNqnavZ3kyaSLPdSyp/IqJbPCE7X8pD6vU/",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 15,
                    "token_value": "Kv5cdaZCuJB4CQYCAFhk+pMAqJ+zFSLNyUxGKfUqKzFhxJqaEaPFzOQWaVk6T9+mh70Hc6NDe9/zY1XkZgdrRBjim24LSGkyqV3R7MwY67vnjKle6gSSNm7xjXK1CxdF",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 16,
                    "token_value": "3dPzc+573MLNwd347aF5kDAfAJ7UkjdGT0M9zF2PSp4zC6dTeZjv9umhsORf3T2/U6dZAt2YwzPvJ8mDRu+FHpKVmbPFHhv5S8/4OprS4Z7bzYVodCsGCQidy5IkU01Y",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 17,
                    "token_value": "TJaui0wQmkMU6OE3rquE6v92Ki0hw1j3axcDYQkqdKqiA9t3yJiPCW4FHEheZjX97G4UpuU27m8NHi4cuk60k0TceFp8d8ucu27e7uMfyApvzfKn3elVOkPdWJAfqWZZ",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 18,
                    "token_value": "wj4zbObP0Ox3gPS4g6uMdE6u3iP0dMfxODroancfjwX28qL5yn+YOX+5tSh8fC6n0I0aHdxpbES0bhtSHcvbvpSM6gPKxinzBjaOJiVJOQ+MhwtatFo3ZSg3wR0lf9d6",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 19,
                    "token_value": "sSyq9gx2RI/1bc8WYmOzBgzFWhlavJsYL+edaPgD3XD9jvDUSl8IjUkBIV+neBVaQNeVHxPMvWSZqW588sUL3H7JEkCAg2Pyrief9hX2b9GxssG9/2AnUnBl8vXkZJRh",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 20,
                    "token_value": "irFw+W+7IUOJaYz9mkJVxyrYNOAjKwjLUQ4RbTvj93jJ3xMJkR9bYvkC89jwx7YeE8ivX/muReGT3AjPGuFhAFylTKAoHHEPSLRRC/ul6n8iEEk8TVb/1rjcR/QX1Gw1",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 21,
                    "token_value": "rAi9xaJ8W2AqxRWToyEvojzZrGm+2AxY6jXy4q9hRIu4+mCEreHFT6uZIlM2s9Hby05aH9sye7vyOjeK7gg1Aua3EM9cwGEiO8HHjArNNIiesM1xQ9LzG4rWFg9udeQ2",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 22,
                    "token_value": "lONxRVJAMOLrI4oDuS7zvOvFunvqIUEWzADSf7ERn1BC2Fh4OhPbly/dMl3+77TjV9iVg4s1XFnBykayrePJRY4kTcZmQfDsuABFjGLV1vn19aubgosMKQD+eYRo3BwL",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 23,
                    "token_value": "jf4O4SVxELGYvt7V8P4qWE0+tDP29iAetSWAM8m93yfF2e+rL2sLW9+U9wBxqWaS2XOptBsc37SSbIn1vnE71swmlKCsrLOGin12LNje4YVLT2wwbdwvs2++a37XO+9d",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 24,
                    "token_value": "ktrzWBMc5F3r0LpZGumncKewUiql6vlfnOuKheD/CKfVCleMepqheyJ3LqsPBX90f/e1IMVPjh5EWnIa4gFTxN7Ebope7xf4YSdCjXHar+tQqUYDVhk7af9HHAhlWHsM",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 25,
                    "token_value": "pUngjFY54/tIisjCo+ciO3VkojovvnfR5JkyEbPWzsA5smHW5v8nqA/NLb9094hVS9RS67yctos1wA5we+uG7Z4x7UQLdL6awQ7rSRSfxfWNZ4AnPblJMNC56orG9vR3",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 26,
                    "token_value": "869AS8Iwt3QXL72Vgh0HC7gdVIP0LCE8M9hHSN7JY/WFGztPem6ciuKa+2TdLsCTiGQw6nyclAuzR3yePD6WHkKat6rhx7xxUGBtm7CYZc8bNj6YC3G1IHt6ysa5KaQZ",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 27,
                    "token_value": "y7mLXDyJAYYhYomVtiH2e+Hl/LV9IA8Wa74M2txXqvzlU8UyWOjxKNSc/mPhbaxO2OjdeY+prcXLX6L9XFNx8ibwPeDxhR7I16Ebopnx1WHax7r5oKMnQ+wCraxE1Wcz",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 28,
                    "token_value": "AmuEdXOLyKWniZ3XXe5fgi7qSOorDKw3LQ8FOFgLnE0Tn8R1vXjfhNVuy4aAtanLT3IFJmJ2/HM4HkqzrWVpgBZ06jYYA8M9Hi+anQoT6+39s0oAUXh4/jIqUdGvy8dA",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 29,
                    "token_value": "q5gaHAetQYOb/bcyUFQHPZEUI8TKzZahzotnow8a8TOzAoIpl010OMwXjwTFXN55wZoeToL2lCboAAQX44N27gZyMcOGw1wIlt7RuZzpLWesdIUPc7YoDl5tQUCo3NBm",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 30,
                    "token_value": "5qxHD3ubLtez+OK91HEycv6erGf60oczrbzL2x0dcPnhPX+g4BQZImjQqpR4+ZY6su3E9eDhWbJxbOVEcrhAPGqdT1zxOhkx4vK2/kCreLaZdTkS48K4WU5R/YUKKaI8",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 31,
                    "token_value": "gMnmF0lkZS1hEAlmTLOI0UpWSIlveywLVUSBHofizvgLpGn/NJReBdavsvueNBsGmndim/pq8IFEcIi8iVKPtWb3gPXamCr7xbBisKXBQjGajf2YrmElVXVAm+ZgCrN/",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 32,
                    "token_value": "zfHMHTQLtlHgHBbLOBts3M6TFDWXMFV0e+aFrb2Q80RKMflYlH0ZBbz4Pqqd5SykyHoHp6UMzAcIfmaFIyioTRgjQVqYpCO72KIJIEvdtsMdNWlVWeSelh3dryfHosd6",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 33,
                    "token_value": "hdKOWJgqlxHl+nQVoUo6s+x9TRsyH9/jvKvtOtPxnlK6+HjI6/25LbVYECel60DQ8kE6TaNRg7jaKg5TlzCDJKCMGySKLyAROaqKTscfVKlOYj5J4ed0EOXQj8spsdAi",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 34,
                    "token_value": "knXoqfIsOAz5HOZskrhAL80eAjxyOEBgEXF+cEAo/h7KOnLDewMnQF9HSLdKHMXUxiHOfUY2Wz6/0IbyhmlKopRJ96vufsWyp5MWDszoR4CAiTTftEPWcJ8VGLMJhlYl",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 35,
                    "token_value": "1i/pHtSye7687JNX5077Lh1ZZ0rVxUWTB3DZn0XbzRU4u1VoBMLfdrQjtJ9IK/7epTflPtmN2eDPgIzSUNNOHxLkeQBIZP7nebVzNrJoR9ZmgfkLi+Fj47hWOCfhxoF7",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 36,
                    "token_value": "DSmOkGtjvMKeLGlZJu3sKf8GjPeJ7AgmQ8b4l1UNzgkPn1V+gYF04BPXefVRlRvupuTS+tfOPD9GfM12MuXerARiMOP/enyXhGOvd9JJR5SRtTcWnSUElwpeEWTdZk1a",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 37,
                    "token_value": "hi7D+gU3AGSnM84rFteRR0TaGF3Wm9RwlbaYeQhE75Q8S2cmDFDrrjaggd0esKVxBIjMg/4ygEEkbMPtNO0qF1BVdTRREwGv0txSnoNZAHa/kZEieIgUvuyPLwrC0ac3",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 38,
                    "token_value": "ZWTRPLBrhqHnHXhVcaq90Sh+LSwOGdQ4arr582QhbzY+5wKdgeDblr0uIKoAoQArBORjctarMvK/ih8SpC+2GgyUGaGxf/USVtX+XWN85mUYtSEnSQ0YryQ+EtyIN5tV",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 39,
                    "token_value": "/EViJ3oXggcOgvkNApLUCoaZKR2+gyC+YV3+r+aU79Rh63tmNCVkQ7aBVnKR9xstNLz49tM+zX0972PtlZnn3GwwM24U8iuvNCLTJ7QqX8bB7PzKHHfeT4jnfWmefYRc",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 40,
                    "token_value": "zY6MlhwoZjamDMu+ULd/oZxXzc9mGUAn2TwFAuwytzMY3eMjHJyKMjzThjtHn3ifUb5u4Rd+SvjAXy9Fa0WgZCwf3XAJrGHlvi4NfH0X4fuFdoBzxpA0qcDxrZampb1Q",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 41,
                    "token_value": "X+KzCt0b0wt7jDDlS5e6HxEPv608lTz04/iKpxh+mFOPgRSgWcXWZVegxw6embJOGmc7SobgkzFqjDVEHPYvpXAIbKEoM57GEHIUlfmxCiX5XpoGbcFFJXVMZJdJMW1C",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 42,
                    "token_value": "N1zjW3KwF9HsC9fWEVNrlGZkKIuyJYYikmhrEE7IVD0UUvel9hzRXh7miTZGGXknpPnvbF7FYGgiLbWiRyJyR06QXEKeinqPJeKSh/Vh4XckYBmh4O333UKLsxthFsdw",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 43,
                    "token_value": "bae5ZDh6EX9dO9KGa/kIlWphHXRa27oDJ0lYBhsaxaGJmV9p8Gnp1OA73jKk+laxsSr3y+OghVC5iK2pMasY9uKbaOujHGkvWUlb+I4aAUjkg9DBHWr23FS/H6oRugp3",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 44,
                    "token_value": "Y1Kl738gfRR5WdDkBUPHz17Q6vgQdPdB55DjNZwF3ROaw4Q2imZ7kdCLKyFcY4qwZAPx7r51POAU1kCZuXl3cEAlyp8piioBcPKxvtNpS5hWkvw25x4gKS5nNg2Sl0MQ",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 45,
                    "token_value": "L3sNAiVPOsLfgvnnu/bIslhY9dGmMv/ISw60zQNaq3xBOvF123dSYuubumhdhN4lWtVfMmZYU2idIqObnOxhCf5sK/5w5KUYQw3907JELESYKoJsaO3RT1UhMFsH7Zhq",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 46,
                    "token_value": "5ASGP2SAfG2NwGuZN8WRda3lmgisO4mrClrqM5PKje5v9NLxHV5/6ngPBYrNAUe/8Rh7YtvM0Zxk0ZtB4KP5nMwanuU+MizxqAECfHZKKrZWxu73pTyRo8jrCsLHpHIL",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 47,
                    "token_value": "2j6QFw23qDajUB4vLpLAedWWOmX4+toWF0XaRUHc48QxmhpA7FvrAvdUS2os8IwQi0oDv31nPT2A0DHbijDYJoBdct7tj41h0h3mPfBcRd/rq/5dKCxSGeqZtotyqtRi",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 48,
                    "token_value": "88OolDXSPvPFvjwgX05hddo7Bi/bisrgNd8Tye6C/D913vnlcXbnBZ0JX1PZMSWnv1eIlSBdU+EKVS56+3Uo0FY/K8GP712bqcCYZgeR+Q5dakU3l2XjXEuEcJQlsCRU",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 49,
                    "token_value": "+pGPgurGTAgRk+nkpBIvhy6rYNbuucF72kgn1blps0Jkvq25c0xzQslCEBNOrPMZnpcEFaUHRZQtzW6oRgnPt+7LyyHzrMmMMsu4WEVLWXFew4ceK6zmwV9FnXQAzXZR",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 50,
                    "token_value": "95/qaBX993PkXYbyhmWcCugik0hmjvLFvJMRuda25sO0I4iJWKeRALkacDDZNR8SwgN+I6YivsrQqbftsHoIE5z8Io6KrI9RKY5M1voHnBAobdyWuN+4RI2QwsgV/KNv",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 51,
                    "token_value": "j9kzDS+0PclJN5FqjTcaMQ0knRTbonw4eBEyZB1eivG9v4AFdiu0VY9kJ+L3s/5Kt3QWPwUAN2CPRTN0JHXfbnyuxJSREerC3wi9Wc4YJovr52z2eWxjO4KZOlGpZJUs",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 52,
                    "token_value": "8pCm72glfQQjlir3D0q+H/edOzM3JFWwCA1SAlF+DoWsYC1gM1kFEO3Fppoi19zbDEfhZrWCAbsQ3qoofBgtpIbfr186VOCkMay0vuE0zwETAsNgzTlZEJ/IEPJGAV8I",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 53,
                    "token_value": "cLwr+p2ugwZ4z2kE/D7/yyBofCzOqa4hVcq4m3fQ1nwl1B5BaFl0w7K8Afp95nb216cy84jwQsIR271DSmlhziCHwmmsGbsDjeQb+c9MIsIK8P8sHqvqroAgnKQolGYn",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 54,
                    "token_value": "a8FRtKMqzXAm37BxWRTdoeOoypOff1hhDP9iYnBNUrUXGFiwiAiB+84c3Q1i2gWiCBe6rPuJ+w+FrVVl/bwOPcopvIBfDmUeh/oXuVk5CguS5LVs8iu7liHgegmg9Z1H",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 55,
                    "token_value": "LFBBdXuvIlV3MjjGVjW1d/sIDkKOj6FQzcg3+UJoy4bqbY9ra22vvCqYP3uF9OFK0WtCVBB17izSRWDF/q7u38Ie5bXdi8aJCBWc4ojsV+LuGdWxC+DihQ3xw/rHFDZ7",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 56,
                    "token_value": "m+zmlS++TH2IhhrMn6JR5DPPfhCXnCDfKs3aCiudvPO9twfFZ35+umZhAYMGTP+FCXMXKg9zhHomIlXQ16mOV8Y5uKJyB1dlYn8cZzj+OuoXjfKZEr7hWmsaflkK1lsD",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 57,
                    "token_value": "u8n7eRBiFXdjNavQ3HhUxsfM2OaGbSqT/uDtk4I0+3NqWnRLA5rtXbNLBF9WIiNY2bl6KfjbDXqpRvkFlcr12vqCwFNFkQ/KEL46St/BDGckqnM30Ywi3FSNFrqG25Z0",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 58,
                    "token_value": "97c+k3OAR7EyY+wzevgyCb9gXQtpaF2V/pbd4pFH0JySZBaORTwHFR1e4e4YZkcBV+dNSGoFP9DD8sNM6UHMJ4D8OCB2+uqff7ryiwTkxxTGaGl3dDdrLepce5t+jtgR",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 59,
                    "token_value": "01dmb5QK3mXT9bo0/24CVThr3RkG4Rws7WZCjsDtcHToLLXP0osswRBWagVzTl/iteejgya4juOTugf1WIX9MfQr6Js6otbkRfUPPcoyn8ZnmQvcBkvALp8x7Jqt2Sxk",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 60,
                    "token_value": "piUWr3PxPO/sY2r0Py+y79OdoScLEr0Z10Dtr5Dy5kjFrwKCgq/KGbjOC1g7gKKVToVdq+pLr1XY+JzWKKmtmZi5ZPrtMkETGRsYpDKlayUAcDyEMyW15sdB/tv7Z2Id",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 61,
                    "token_value": "2RXcRpVnKIwkvhl9+h6Nv/b7vOiWZZy8cXd6tccADVVg/EYVeXUaxcdtijx6D69yW6BtkCgYsKlWD6Lvfe6DVfCJBDZ47OUmhki25ZykexHXrN9coe0WBdS/jMXYL6Fh",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 62,
                    "token_value": "z0oROg16yc3WZXxdsxfnuOxO8w60u3cHDfne4wLwlZarglweuFAd2TgiIkFYTo3dq3C2S/BDItjawSGd/xMYutIzMAIYPHfFoF6bQW8GLhA36leM5xA9wbG20+vCvVIt",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 63,
                    "token_value": "ApMIYnoGzamZ0Ir+ufrfT7+TTox0YcJ0alDXU0EM/PeSdl6U7dwvQfq5WZfkVvJFj9WTK4q8ZbtuxNd89ny80bB8PY7YYmX60jRgJH6Lum47VAFZfZH4sU92YWtF5YsE",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 64,
                    "token_value": "+SxPfE+CqM4zHRPXMZAnaMN8N9EZWDO6Hq1cj2BYdnAeJb68hyDtMF8GHUeiB6VQpZGRHm0AI/TTiK8morOSxgoMscJCaW1XM/SCwPBtUVsP8+qBDie10kqWZ6XIMuxB",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 65,
                    "token_value": "y5QkYtMDQd6j5uYo4jBMLOH0uHK4KRAvQifaBlaVxTOP07VQfYwQ+1svRAUeOyazNIBaSKF4EDxT8RhrnUhlOKZKxvoQAIS1lR8zZ0CaNgokO0nONkv82hIiyOn0qhI8",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 66,
                    "token_value": "pTFiXShYgGtZQG6aPRkqJ3m1Yu3QhQk8somHrxc03oTzstU0UTo6EZYSoMfVWSjw870BW6jw7Wav4Kz3MHnUTYIDjT6v51uGHOLHpPxjxaT5hBLwBcIV+1B9U8SH/TQp",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 67,
                    "token_value": "9vqHTUialWdw6iyzIEwLVHSRrEMmWUTpbCE9Gw6GG4s5Yrc0cQ3cto3THd1adnWhw+E/Wvv+/0slvZKNcEzXru5IuyBkiGPEN6lFdrpa1S/m76cZ1ywWtcrnBenZBuZB",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 68,
                    "token_value": "fXJhdjSIoHugY0NO2t5e8PMffrS3mpO6tbkzuBWBnIn6XCxfL5+pRA42UmJBDNe9/s4mXbxreg9/Ok5DxguaQA7uI7psM6oEF63lMsvMtbjtHDUs2ajOk6KODgRRgFwf",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 69,
                    "token_value": "O9MkmQqo5OQN+w21f2V2ghlCmyCEzNtvEYdxliD3CZXNoQTLaJtK7j9gtby0Uf21Z5JRK36WS8eJ5xtzPvVr67qqXoJ7qsrk3ePsag8chXYb8K1zNRpAUwQ9o6kTKAgy",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 70,
                    "token_value": "vc/sGBraLrt8sgFIdTf5dBM6ySXZ19ayxHH3NpnYn9ooKi9dQkYsWVJ0PxOo6D8XDsAfgH6stKHzhUJY+f4fc0p6HBuLvN7zaUY1kstBM3EsB0fbSDSm3wToPhlJIbwI",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 71,
                    "token_value": "+oFPqEmSTyx4IjjlhaVwQDFL76qra8cJVOJ8JKRuR+79wk5XDgI6wCJCiYHb4sOKxHe17Tov9cnl4PGO1GLoM4oQvAhc+8ilRU1zY9Gj43CLKZK4bMG35slzCX3GSEdl",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 72,
                    "token_value": "2BRhs8B/SdTQVNcLM4L907Vcw1hvNC12vAw3z7RVwLje93wN+c4vNguD23LDiskd5YrXzY+jP2716SlekzSuP3BbzB31Kj/Q14NMVuoVKIL4111qIJ/pPoCOpmHD3fpe",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 73,
                    "token_value": "mgDqKIh+MvaBVeRSUUFTpAeNTuqy68/Kuf6KGecA1qozJlQJn42/VPH2b2jqsaOFgmzlU6TXM3HXg0mhrQwKWkp55aJK5CQWPBZ4G1G3p8QosPUyMCeV0t+BOGVO5u9b",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 74,
                    "token_value": "ys4WCTqhTVazgQnCFpzv4rrN+N8++w9WENR6UIh/FRfkoi4IJzUgqmOGAnXRuOBuM0CljWtJUEmS9CK3Xr+qwOzADbW3BtFfVkJ5ke6nMOVmz+QUgpo2IhPKoCn4hplI",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 75,
                    "token_value": "kHRAv7lTFpyKrx2osM9oMdrriv1tHf5OAMNpYqE/QEIm14hp1qksbyz9fO2J2X454GAFlDIuuMczHZWzSaFmlaSJ1xgOnV06TUTu+PlTNBRGd5jOgakc6ZtGN+3/b3hP",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 76,
                    "token_value": "n2qQelZiw7YMi6ietk0u6MuMUno4i3RAH0tVCv6b248UkOJmuByX9UchaDJ+b+HgOCkrRAAf/mtjc75ZTRKmaip0EGGDGdTskeITwcp3wjHPgFarEJ7ijeLf137ISXx6",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 77,
                    "token_value": "294sJ+XBsXUiEmTsCm639c+EnNjMfn3mPVZxuXKuFvz6iS/U+kp+kJvJKP5ZvInVvzHXB2sGmuMLJi39GvP+yODUt4SYMndobH+NJ//hhNaYfygesLD832oby8JKZDZ3",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 78,
                    "token_value": "jljPEwmzGusnhzE0HmkVgDQ88AwSf8D5YdFkwFJvQO3pysMcyX2f0vtaYtageeYW49S7aba0lXLKlURXrjoa1maZ1mhDXS7PVthhqPCVZvXlpN/lNTzv5mJUKHv/RI0U",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 79,
                    "token_value": "YUdHhNdOlOJo6kLh8bwBgGqfe9pGo+i2+GnJOW4MRciLG7Zj76vdn+yUlvZhMYCTSsBTxqCMK7s9V6pDwMJhAkATJkAE9o6ciJX2Opoyi7MW+9CT7uGVY/hzgsXHJTZH",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 80,
                    "token_value": "cWXUSDuND4XXKbq4kmuwxFhyYUaiuDhC/qJy04tQfK8N1eWvWMPUCGecwgXW3cKYmLHzs3P6mxvmlWov8JICe6RQ5q2L81Es/VGPRqoCuAG6/BmKbVjXlywiWv2AirdW",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 81,
                    "token_value": "vVl45liDUWqoaB+H4paROTZG9pFKi1Mmx7i8etBnEpbglFoWOl10e/Ik+px+B9YPsTg0M0UYhAfKSbbfuYszrYTFsuuY8wf11OHv5yKdSpLsswLXJG/FgUvn3rvLKPJQ",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 82,
                    "token_value": "F0lL6yAyg9xg+GiX6jBguOAonRrPHO910YIDr2ndjKb3x650qGFa7QnoypAghxX6Vi+Oi/pU5Q7fYLeT91NXyegJcIwEFgNMl+0wpPrxzb48rk2/Fri22fgTdcm7hBd0",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 83,
                    "token_value": "BJW7siI5KDbhVLBO107kFLadFzTocsFwcLYqzhrXiaEbDxphF48btVQeHbc3sqYtFg/agsOBLGffD1Y5HF4xk57rwDY8iZa+tTqXSvujehZdxB7ysLzaWEAouiQQPoI+",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 84,
                    "token_value": "7V9iQPExEnqhrTY/1rc6ciFdsCSVhP3AiWVfMtwkDusmYP5TeCpgs6ARPYnh6Yu9S6+nQDd5o5AH3rJcfKDGmKT2NluXV2GFXlkQ4f3qSuXpSB48fA/Grzo8jifOXkwm",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 85,
                    "token_value": "tiLNaSgUD/pJPHxhSf5l02VxtbAtDciTMq9VJMApqIOSKCeqE5/QjB7eLrD58lyMMXvVXvrroa5deZc9WFhWYh4FNogvOIC2O774bPywTqoAcnMzGhVK822d2TU1Opgd",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 86,
                    "token_value": "+dKLi0jYn5tbmi661dvJgoNBJFVO4VRsaNplrsLzVd45A0gkKv7/WeB5NIQQto3PXbzHRt7G771X5RvOfXwou8xAnULAXw9L1HWqYQU6NENPRmq767Mpen9UyEt5zO9u",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 87,
                    "token_value": "PxI5jLZ8y/uxLSqz7sbDOiloIzCFMQk1kQcC3Ujbh656tDICaD+Abcag8QMgmeWm9qnqOkj63rQfHC7A2TkowL5w9/X0VhPAfmxgZNMqtotE5fbdT9IbPOz9gec22gwT",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 88,
                    "token_value": "f13sOD0+Kv8zzk7oEAvej8ST6kXiNXw5TyIl1OFZGgro8inHldye55amVCCTZOWv/8n5bhWgwihjF0g8JBBEDbDEaN7RO67UYvWYnJVomEsJ+muB8Elc+ULJasT85LA0",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 89,
                    "token_value": "/YqLrFHoxT4pm7xeFFHo+BscZ7mQVO40gyb7jFWZ5EF5R+IsnWZ60CH5/RAeoOPSn2oypwVGem1cKJV3VNduN1JBrbit7TUrEKXlOhf+lYfRtLca4ykwNKI+cPYT9Z4x",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 90,
                    "token_value": "GaWCVu0krvv07bcTVngEb5eKJlitc89T1E9HTkIfXDHa7wPQ1m5hpNc35YEP96OXccyPsGZjkFmwjvtt3mt4YVTeo0m63rxO+khByW4PtlZ73y2UeKXT3D6LJ8nlh/tI",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 91,
                    "token_value": "hm7tBG9jkK0bhKpqhZeTfM1kIzial/l6kg5vk5VuzFIeni6q4vY0w5P49FHNfyopXC2zXwIHh/MAs7x6DiqjobS9Vg+O/BK2bbNBM2osXsmdGic71yguUD3cxqc+WZl4",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 92,
                    "token_value": "A92rB+0hJI6XmCF00rdw/bVYTdGqqFXB1ShE6t+k+vPzl0/2pKNkhwzTwcnn3Bg77BR2DkcGEAo2VDCmmrn3inis48aq50ooZFMuyYdUxkwAVPyujuQ9/k5dVat8pe0v",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 93,
                    "token_value": "UwNITzt0kmQIAIbsUvTs2xNWYlJgkdphxG0cKgRU1lmVgL5Un9cjXdEeWjFi0O2TbTQf+oAqrSkg9AY0YIr7Wyq3oMjqtGEf2GBaXHD5ZqMpE0gNCdExSWjY6kVy0+Na",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 94,
                    "token_value": "WS7hXhTRex5KqXQmtRsbLj3cwYwoUOlxuDlBXP2qU6SYWFtzri+bEt43Z20P90OXpdMNbE2tyl0wWLQYUGbpPMif8WIIkONy1nzJfytTlZ3OgkrliQfWfm84v3jwW1Zf",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 95,
                    "token_value": "RnVJa/gh3XAk7igvtjmjmBY1Cao4HkMX4RA41flwCq9YskL1El2gY3JRKGLZY2zqsd6SsPOwf+bZTN+aRQvpC4yfxJZ/8k1ZqgB6bcZBKETHbPDzjFuJa/w/8vtBoEtg",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 96,
                    "token_value": "Qxjj3LeevyR8SYkiq+ciWQT0dU2i3GJDepwwWchCpDQBCqgQvIrtAbuKt1V3MIpC+ZX7SwEHBdCH/otJCrZXg7T9Ksm+LNcejBLjtTxFbMB/klGzIfZqBN4/j1hItQNX",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 97,
                    "token_value": "rjw+2ltASz2WZDBNIEE7zB65z8QiLF0z5LvkXidZuATS7GPr+QtGiXB5y92tG4whwEhJe06voKfcDRNe7qSD70xSiTCoAB7M6tICK6f3+rZ3GfA/1EVktyQ0onEj5ycD",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 98,
                    "token_value": "+8oCeIcBiUSDboDMnl/8iRkj7ka+YWYc4E+hAIaFdMELKw6zCH++1AcqL6nx6ReyXuJ4iIcJKPtCCp3ueloYk5x18+3SeRO3nNG0u5kyJwMpkive6jV76hyBcklP/mhg",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 99,
                    "token_value": "LoooAbz1YC7xpz+CupaitPahCn4gLzYjPMi8SsIlcw32ghmTVuyZnbnZ7e94/BGj12g+M2Be3HGpba+O4TF/xaBfsHrP3bVpkzeLi/IS/1K0jSUkgiEhGxPz2lf3XXwa",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 100,
                    "token_value": "PKhs3D0ntriOLRIzfo+lKQ4Tv2Rnfu2Stjno/iI9IxiChS3LXSSuD6Y5erqtvqvHd258qJq2WnkXSrEE+y+3bRB4uhOE1aCJgMkp6yR9Ru94IZU7BppXI132g4E84CpK",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 101,
                    "token_value": "cCbM0peu1Tm7UDh7tNkt6o0aoX12WqqP0gN1OHNWIjhpii2Q0OdMwcv/3rg3wKudHcOOXPu+8KAB30qBhZeTUS7oZb0bSLgLvqcW34aUzsS8SZhzC/pipbBA1HGN4QwN",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 102,
                    "token_value": "xZIRyUGRE5yPayaQoVAWf8hqcwROSNjHdk6VBq4/mff9VQtCEgdOrs593qQoOxZITKcwe1yvCVWkKaz2DGYPvSRG3cEl8tTJyjTY63K+Ks1m/C3oYi0W6BsfHGVKy4Jk",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 103,
                    "token_value": "/Z9uP3T2Ze0I4GwFlvuSEPjDJauJPr0/fHXPO/5ov+rrCUo+945T47ooa6U9Wk/pLnq6ZTpIklkqJLnjMhLp8Ny93qq3ujcrNSswm3ETsuIZRKD3eZpkUZTHzsZPHck3",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 104,
                    "token_value": "CCm8gUCgUrhcGN4jp/g3tfV+QVnDT8xAd5txAnS+P+H6UK9NI79sZO37eAFQWLQri+F17sCk2REZMqVMmLDTvZ6VKHwzMPofqjx8uM7nijNqhq2IMOR4GIDlVBvrmcBR",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 105,
                    "token_value": "0uWXBlX7G4Jd8uvEaHh6p1YikOhPCXRLAisqQ4tJYW/WZ2BRPrTcGLP8gMpJpt0E9+egCjPAjmBi6nA8ARhxDJbR+tHqkBVJmjWLCE7c174SX+t3MAL0vKV0NKTC3aQD",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 106,
                    "token_value": "95uryeWmcKl0P4U3aFDXK7jFCOKCjMzKJUOGfV6VHfb+J3CrL+/mqT9w6++E2xiZo9LAmMxS9vGOmAYFI0mrsigKRLb13o8UuSICqsFot+EWMj89iTnFrX1+IwKq4clQ",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 107,
                    "token_value": "JBvB+bFAb2bSQ94PJzL9XsRW1gR2j0IXoKbERg4JHf7rYvsc4VS0qxEYSXZwmuJE2ra6Zz4iW9Vfu8HxLf5BYYYg1GhqZt0EWEhkKW2+BTx8f+vOyLU56+Vc7rQGaEQn",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 108,
                    "token_value": "Vn5GsZlKBTRzsGuIEbJuJ/VXldPOWWx0NSHEM1hHnSa2tYoHY6lffe+cuw4URtHKYdTOoeW3xkQ7I1quosk8FgCc0Oq6ABQtT8a0PEKjaMWHL3SK6re1ITiEumaLJeYb",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 109,
                    "token_value": "8GMVYpbdN7MyVzbv5tUukyupSpX1k9glmAFw6Iwvt1+/xWoawYXCvMRRx3gFxBhG9TuiADkit+H8LBN51VI2a4iiwLypzrrT5syuWtn0Yh6oUCulHYeITtDTTCjuNS58",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 110,
                    "token_value": "KOEVEmTsa3jaYukwuCgtkzP1zRA5A99uwDNd4QLeNPhU3bqj9VYDxryJznH+VM9HV+P2A0mlNcuB1qavSoBu5hrV4wZLpxioogsmb/eBPqSbN7yERUQhRSZtSDCEqZZ8",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 111,
                    "token_value": "xJ4Q75x9Hz11LpRf41G9wspQxmuvoj+j16QZuQr9V2mHqn8JrTXdKzH+5yjcmwO6TRtccEbmAdOKPSwfwOgRPQJo5wXcvIc+86Ci6POQLFkVWMv0nOBtwIdzSj6msIhi",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 112,
                    "token_value": "3KztsHw+JS2DivHQackXH8bgHDwDuZ+qALj6WeUbtV0SuITL7ZVp+gveA+kAwRnlJlcupu50+82zffXTOY3gRhiPQE0vzxb705cvqhusyulbWwuu5ZhUzGfSP2xJKV96",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 113,
                    "token_value": "W+lG/ySi246inyZ42ZzSLsf1yUytL3gkKkkN2JeRfxyDDCUK2huwThCHALhmjN80DCHOtoQxpNrQRrl3sTTpyISXe/SH/GFU7c6g3irDsLQvEnmU9dtDGw8ruoSwtuMj",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 114,
                    "token_value": "w9LLwmPEweQM2MocpSvhxEw/GoeOR+8padiV/nUd50WUQCVmfZxZDmgl5wjeLfX2Pspw4ualBy4kmIH1L1STaLwvueqgf6FJQQizSZpXWLLxgM7GaROIWPOooekiX396",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 115,
                    "token_value": "9JlvN6SjWD7zc0pP72qOXEMZAUqut7Ig93np8/6rNTduvuAjF/7xWCDvwvTJ+IK4zrXCK00JDGSqdICCXumiz367rT8HyuNk18HKNzNsQuqTrsHw6eo+4a05GGilbJ4t",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 116,
                    "token_value": "Y6VQ6+sn/QRbEQQ28dptrztZDDEj9Hb8U5L+O/qurmv6gH/aOYOAM4K98QY169EVJxi3jmvCCUd/8Uoh+6MFYfgd1GEenc/EBNdie2XwscdfYmwhPPdI22EOXx8vx6U2",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 117,
                    "token_value": "BXRJo4omZ9MuUy+CD3WUpqD057WrWj+nViaDnkjqrX0x+VwrVCWVBW1H+XIOGRl1EyNcdbSqUmXp+rnPAmlqn5KVdwsuoTGIk6PBKevd1E20MhHeqSbFsXk4Q8G++phO",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 118,
                    "token_value": "iHBL0iXV8PIm+s4AnPmZp+79eNZ3DA/N2ofPUMGaZT03yTpvGAOYjnhkA0BmfekVhRTUAQj6gsjnEYAvm7HCxuYF9mZbbq6Xet+jlKaBD6JX/HiV0yD4h0T5tOCPEl5Y",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 119,
                    "token_value": "Bj7z95I2E+0xeUfNYE9aOkYMfdddK1YXdFvMq0zypIad+k4IeMMffc+gkjB/eshPr7THx17uAlygPHebMdNnh/JHsAQBeM969lfACJey+EDY/k9fdU1Do10I5CW0ZD4/",
                    "value": 0.25
                },
                {
                    "expires_at": 1645736183,
                    "token_id": 120,
                    "token_value": "No46skz6L9a8bTmCcj30TjuqPKua1t7rZMNfERcFSTNsqf9m6mew5Ix11nDezLzakkrC3Sz7mVWLiZc3mNXTIujGXYI9zCVO2KAngPUDUh1lAkJ9+Uo2V8uYdKhOZs1o",
                    "value": 0.25
                }
            ],
            "trigger_type": 1
        }
    ]
}
        )",
// ---
        R"(
{
    "backed_up_at": 1640598940,
    "vg_spend_statuses": [
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 1
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 2
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 3
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 4
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 5
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 6
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 7
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 8
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 9
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 10
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 11
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 12
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 13
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 14
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 15
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 16
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 17
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 18
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 19
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 20
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 21
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 22
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 23
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 24
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 25
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 26
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 27
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 28
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 29
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 30
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 31
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 32
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 33
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 34
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 35
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 36
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 37
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 38
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 39
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 40
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 41
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 42
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 43
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 44
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 45
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 46
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 47
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 48
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 49
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 50
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 51
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 52
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 53
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 54
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 55
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 56
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 57
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 58
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 59
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 60
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 61
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 62
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 63
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 64
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 65
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 66
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 67
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 68
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 69
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 70
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 71
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 72
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 73
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 74
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 75
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 76
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 77
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 78
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 79
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 80
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 81
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 82
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 83
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 84
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 85
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 86
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 87
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 88
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 89
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 90
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 91
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 92
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 93
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 94
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 95
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 96
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 97
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 98
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 99
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 100
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 101
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 102
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 103
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 104
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 105
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 106
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 107
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 108
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 109
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 110
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 111
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 112
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 113
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 114
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 115
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 116
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 117
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 118
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 119
        },
        {
            "redeem_type": 0,
            "redeemed_at": 0,
            "token_id": 120
        }
    ]
}
        )",
        std::move(callback));
  });
}

bool LedgerImpl::IsShuttingDown() const {
  return ready_state_ == ReadyState::kShuttingDown;
}

void LedgerImpl::GetBraveWallet(GetBraveWalletCallback callback) {
  WhenReady([this, callback]() { callback(wallet()->GetWallet()); });
}

std::string LedgerImpl::GetWalletPassphrase() {
  if (!IsReady())
    return "";

  auto brave_wallet = wallet()->GetWallet();
  if (!brave_wallet) {
    return "";
  }

  return wallet()->GetWalletPassphrase(std::move(brave_wallet));
}

void LedgerImpl::LinkBraveWallet(const std::string& destination_payment_id,
                                 PostSuggestionsClaimCallback callback) {
  WhenReady([this, destination_payment_id, callback]() {
    wallet()->LinkBraveWallet(destination_payment_id, callback);
  });
}

void LedgerImpl::GetTransferableAmount(GetTransferableAmountCallback callback) {
  WhenReady(
      [this, callback]() { promotion()->GetTransferableAmount(callback); });
}

void LedgerImpl::GetDrainStatus(const std::string& drain_id,
                                GetDrainCallback callback) {
  WhenReady([this, drain_id, callback]() {
    promotion()->GetDrainStatus(drain_id, callback);
  });
}

void LedgerImpl::SetInitializedForTesting() {
  ready_state_ = ReadyState::kReady;
}

bool LedgerImpl::IsReady() const {
  return ready_state_ == ReadyState::kReady;
}

template <typename T>
void LedgerImpl::WhenReady(T callback) {
  switch (ready_state_) {
    case ReadyState::kReady:
      callback();
      break;
    case ReadyState::kShuttingDown:
      NOTREACHED();
      break;
    default:
      ready_callbacks_.push(std::function<void()>(
          [shared_callback = std::make_shared<T>(std::move(callback))]() {
            (*shared_callback)();
          }));
      break;
  }
}

}  // namespace ledger

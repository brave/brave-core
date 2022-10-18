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
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "bat/ledger/internal/publisher/publisher_status_helper.h"
#include "bat/ledger/internal/sku/sku_factory.h"
#include "bat/ledger/internal/sku/sku_merchant.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

using std::placeholders::_1;

namespace ledger {

LedgerImpl::LedgerImpl(LedgerClient* client)
    : ledger_client_(client),
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
      uphold_(std::make_unique<uphold::Uphold>(this)) {
  DCHECK(base::ThreadPoolInstance::Get());
  set_ledger_client_for_logging(ledger_client_);
}

LedgerImpl::~LedgerImpl() = default;

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

template <typename>
inline constexpr bool dependent_false_v = false;

template <typename LoadURLCallback>
void LedgerImpl::LoadURLImpl(mojom::UrlRequestPtr request,
                             LoadURLCallback callback) {
  DCHECK(request);
  if (IsShuttingDown()) {
    BLOG(1, request->url + " will not be executed as we are shutting down");
    return;
  }

  if (!request->skip_log) {
    BLOG(5, UrlRequestToString(request->url, request->headers, request->content,
                               request->content_type, request->method));
  }

  if constexpr (std::is_same_v<LoadURLCallback,
                               ledger::client::LegacyLoadURLCallback>) {
    ledger_client_->LoadURL(
        std::move(request),
        base::BindOnce(
            [](ledger::client::LegacyLoadURLCallback callback,
               const mojom::UrlResponse& response) { callback(response); },
            std::move(callback)));
  } else if constexpr (std::is_same_v<LoadURLCallback,  // NOLINT
                                      ledger::client::LoadURLCallback>) {
    ledger_client_->LoadURL(std::move(request), std::move(callback));
  } else {
    static_assert(dependent_false_v<LoadURLCallback>,
                  "LoadURLCallback must be either "
                  "ledger::client::LegacyLoadURLCallback, or "
                  "ledger::client::LoadURLCallback!");
  }
}

void LedgerImpl::LoadURL(mojom::UrlRequestPtr request,
                         client::LegacyLoadURLCallback callback) {
  LoadURLImpl(std::move(request), std::move(callback));
}

void LedgerImpl::LoadURL(mojom::UrlRequestPtr request,
                         client::LoadURLCallback callback) {
  LoadURLImpl(std::move(request), std::move(callback));
}

template <typename RunDBTransactionCallback>
void LedgerImpl::RunDBTransactionImpl(mojom::DBTransactionPtr transaction,
                                      RunDBTransactionCallback callback) {
  if constexpr (std::is_same_v<  // NOLINT
                    RunDBTransactionCallback,
                    ledger::client::LegacyRunDBTransactionCallback>) {
    ledger_client_->RunDBTransaction(
        std::move(transaction),
        base::BindOnce(
            [](ledger::client::LegacyRunDBTransactionCallback callback,
               mojom::DBCommandResponsePtr response) {
              callback(std::move(response));
            },
            std::move(callback)));
  } else if constexpr (std::is_same_v<  // NOLINT
                           RunDBTransactionCallback,
                           ledger::client::RunDBTransactionCallback>) {
    ledger_client_->RunDBTransaction(std::move(transaction),
                                     std::move(callback));
  } else {
    static_assert(dependent_false_v<RunDBTransactionCallback>,
                  "RunDBTransactionCallback must be either "
                  "ledger::client::LegacyRunDBTransactionCallback, or "
                  "ledger::client::RunDBTransactionCallback!");
  }
}

void LedgerImpl::RunDBTransaction(
    mojom::DBTransactionPtr transaction,
    client::LegacyRunDBTransactionCallback callback) {
  RunDBTransactionImpl(std::move(transaction), std::move(callback));
}

void LedgerImpl::RunDBTransaction(mojom::DBTransactionPtr transaction,
                                  client::RunDBTransactionCallback callback) {
  RunDBTransactionImpl(std::move(transaction), std::move(callback));
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
}

void LedgerImpl::Initialize(bool execute_create_script,
                            LegacyResultCallback callback) {
  if (ready_state_ != ReadyState::kUninitialized) {
    BLOG(0, "Ledger already initializing");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  ready_state_ = ReadyState::kInitializing;
  InitializeDatabase(execute_create_script, callback);
}

void LedgerImpl::InitializeDatabase(bool execute_create_script,
                                    LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  LegacyResultCallback finish_callback =
      std::bind(&LedgerImpl::OnInitialized, this, _1, std::move(callback));

  auto database_callback =
      std::bind(&LedgerImpl::OnDatabaseInitialized, this, _1, finish_callback);
  database()->Initialize(execute_create_script, database_callback);
}

void LedgerImpl::OnInitialized(mojom::Result result,
                               LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result == mojom::Result::LEDGER_OK) {
    StartServices();
  } else {
    BLOG(0, "Failed to initialize wallet " << result);
  }

  while (!ready_callbacks_.empty()) {
    auto ready_callback = std::move(ready_callbacks_.front());
    ready_callbacks_.pop();
    ready_callback();
  }

  ready_state_ = ReadyState::kReady;

  callback(result);
}

void LedgerImpl::OnDatabaseInitialized(mojom::Result result,
                                       LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Database could not be initialized. Error: " << result);
    callback(result);
    return;
  }

  auto state_callback =
      std::bind(&LedgerImpl::OnStateInitialized, this, _1, callback);

  state()->Initialize(state_callback);
}

void LedgerImpl::OnStateInitialized(mojom::Result result,
                                    LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to initialize state");
    return;
  }

  callback(mojom::Result::LEDGER_OK);
}

void LedgerImpl::CreateRewardsWallet(ResultCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    wallet()->CreateWalletIfNecessary(std::move(callback));
  });
}

void LedgerImpl::OneTimeTip(const std::string& publisher_key,
                            double amount,
                            LegacyResultCallback callback) {
  WhenReady([this, publisher_key, amount, callback]() {
    contribution()->OneTimeTip(publisher_key, amount, callback);
  });
}

void LedgerImpl::OnLoad(mojom::VisitDataPtr visit_data, uint64_t current_time) {
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
                         [](mojom::Result, mojom::PublisherInfoPtr) {});
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
    mojom::VisitDataPtr visit_data) {
  if (!IsReady())
    return;

  std::string type = media()->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
    return;
  }
  media()->ProcessMedia(parts, type, std::move(visit_data));
}

void LedgerImpl::OnPostData(const std::string& url,
                            const std::string& first_party_url,
                            const std::string& referrer,
                            const std::string& post_data,
                            mojom::VisitDataPtr visit_data) {
  if (!IsReady())
    return;

  std::string type = media()->GetLinkType(url, first_party_url, referrer);

  if (type.empty()) {
    return;
  }

  if (type == TWITCH_MEDIA_TYPE) {
    std::vector<base::flat_map<std::string, std::string>> twitch_parts;
    braveledger_media::GetTwitchParts(post_data, &twitch_parts);
    for (auto& twitch_part : twitch_parts) {
      media()->ProcessMedia(twitch_part, type, visit_data.Clone());
    }
    return;
  }

  if (type == VIMEO_MEDIA_TYPE) {
    std::vector<base::flat_map<std::string, std::string>> parts;
    braveledger_media::GetVimeoParts(post_data, &parts);

    for (auto& part : parts) {
      media()->ProcessMedia(part, type, visit_data.Clone());
    }
    return;
  }
}

void LedgerImpl::GetActivityInfoList(uint32_t start,
                                     uint32_t limit,
                                     mojom::ActivityInfoFilterPtr filter,
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
  WhenReady([this, callback = std::move(callback)]() mutable {
    auto params = state()->GetRewardsParameters();
    if (params->rate == 0.0) {
      // A rate of zero indicates that the rewards parameters have
      // not yet been successfully initialized from the server.
      BLOG(1, "Rewards parameters not set - fetching from server");
      api()->FetchParameters(std::move(callback));
      return;
    }

    std::move(callback).Run(std::move(params));
  });
}

void LedgerImpl::FetchPromotions(FetchPromotionCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    promotion()->Fetch(std::move(callback));
  });
}

void LedgerImpl::ClaimPromotion(const std::string& promotion_id,
                                const std::string& payload,
                                ClaimPromotionCallback callback) {
  WhenReady(
      [this, promotion_id, payload, callback = std::move(callback)]() mutable {
        promotion()->Claim(promotion_id, payload, std::move(callback));
      });
}

void LedgerImpl::AttestPromotion(const std::string& promotion_id,
                                 const std::string& solution,
                                 AttestPromotionCallback callback) {
  WhenReady(
      [this, promotion_id, solution, callback = std::move(callback)]() mutable {
        promotion()->Attest(promotion_id, solution, std::move(callback));
      });
}

void LedgerImpl::GetBalanceReport(mojom::ActivityMonth month,
                                  int year,
                                  GetBalanceReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    database()->GetBalanceReportInfo(month, year, callback);
  });
}

void LedgerImpl::GetAllBalanceReports(GetBalanceReportListCallback callback) {
  WhenReady([this, callback]() { database()->GetAllBalanceReports(callback); });
}

mojom::AutoContributePropertiesPtr LedgerImpl::GetAutoContributeProperties() {
  if (!IsReady())
    return nullptr;

  auto props = mojom::AutoContributeProperties::New();
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
                               LegacyResultCallback callback) {
  WhenReady([this, pass_phrase, callback]() {
    wallet()->RecoverWallet(pass_phrase, callback);
  });
}

void LedgerImpl::SetPublisherExclude(const std::string& publisher_id,
                                     mojom::PublisherExclude exclude,
                                     ResultCallback callback) {
  WhenReady(
      [this, publisher_id, exclude, callback = std::move(callback)]() mutable {
        publisher()->SetPublisherExclude(publisher_id, exclude,
                                         std::move(callback));
      });
}

void LedgerImpl::RestorePublishers(ResultCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->RestorePublishers(std::move(callback));
  });
}

void LedgerImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    mojom::VisitDataPtr visit_data,
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
                                    LegacyResultCallback callback) {
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
    auto info = mojom::RewardsInternalsInfo::New();

    mojom::RewardsWalletPtr wallet = wallet_->GetWallet();
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

void LedgerImpl::SaveRecurringTip(mojom::RecurringTipPtr info,
                                  LegacyResultCallback callback) {
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

void LedgerImpl::UpdateMediaDuration(uint64_t window_id,
                                     const std::string& publisher_key,
                                     uint64_t duration,
                                     bool first_visit) {
  WhenReady([this, window_id, publisher_key, duration, first_visit]() {
    publisher()->UpdateMediaDuration(window_id, publisher_key, duration,
                                     first_visit);
  });
}

void LedgerImpl::IsPublisherRegistered(const std::string& publisher_id,
                                       std::function<void(bool)> callback) {
  WhenReady([this, publisher_id, callback]() {
    publisher()->GetServerPublisherInfo(
        publisher_id, true /* use_prefix_list */,
        [callback](mojom::ServerPublisherInfoPtr info) {
          callback(info &&
                   info->status != mojom::PublisherStatus::NOT_VERIFIED);
        });
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
                                   mojom::PublisherInfoPtr publisher_info,
                                   LegacyResultCallback callback) {
  WhenReady(
      [this, window_id, info = std::move(publisher_info), callback]() mutable {
        publisher()->SavePublisherInfo(window_id, std::move(info), callback);
      });
}

void LedgerImpl::SetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    bool enabled) {
  WhenReady([this, platform, enabled]() {
    state()->SetInlineTippingPlatformEnabled(platform, enabled);
  });
}

bool LedgerImpl::GetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform) {
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
        [this, callback](std::vector<mojom::PendingContributionInfoPtr> list) {
          // The publisher status field may be expired. Attempt to refresh
          // expired publisher status values before executing callback.
          publisher::RefreshPublisherStatus(this, std::move(list), callback);
        });
  });
}

void LedgerImpl::RemovePendingContribution(uint64_t id,
                                           LegacyResultCallback callback) {
  WhenReady([this, id, callback]() {
    database()->RemovePendingContribution(id, callback);
  });
}

void LedgerImpl::RemoveAllPendingContributions(LegacyResultCallback callback) {
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
  WhenReady([this, callback = std::move(callback)]() mutable {
    wallet()->FetchBalance(std::move(callback));
  });
}

void LedgerImpl::GetExternalWallet(const std::string& wallet_type,
                                   ExternalWalletCallback callback) {
  WhenReady([this, wallet_type, callback = std::move(callback)]() mutable {
    auto on_generated = base::BindOnce(
        [](ExternalWalletCallback callback, LedgerImpl* ledger_impl,
           const std::string& wallet_type, mojom::Result result) {
          if (result == mojom::Result::CONTINUE) {
            result = mojom::Result::LEDGER_OK;
          }
          std::move(callback).Run(result,
                                  wallet::GetWallet(ledger_impl, wallet_type));
        },
        std::move(callback), this, wallet_type);

    if (wallet_type == constant::kWalletUphold) {
      uphold()->GenerateWallet(std::move(on_generated));
    } else if (wallet_type == constant::kWalletBitflyer) {
      bitflyer()->GenerateWallet(std::move(on_generated));
    } else if (wallet_type == constant::kWalletGemini) {
      gemini()->GenerateWallet(std::move(on_generated));
    } else {
      NOTREACHED();
      std::move(on_generated).Run(mojom::Result::LEDGER_OK);
    }
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
                                  LegacyResultCallback callback) {
  WhenReady([this, wallet_type, callback]() {
    wallet()->DisconnectWallet(wallet_type, callback);
  });
}

void LedgerImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  WhenReady([this, callback]() { database()->GetAllPromotions(callback); });
}

void LedgerImpl::GetTransactionReport(mojom::ActivityMonth month,
                                      int year,
                                      GetTransactionReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    database()->GetTransactionReport(month, year, callback);
  });
}

void LedgerImpl::GetContributionReport(mojom::ActivityMonth month,
                                       int year,
                                       GetContributionReportCallback callback) {
  WhenReady([this, month, year, callback]() {
    database()->GetContributionReport(month, year, callback);
  });
}

void LedgerImpl::GetAllContributions(ContributionInfoListCallback callback) {
  WhenReady([this, callback]() { database()->GetAllContributions(callback); });
}

void LedgerImpl::SavePublisherInfoForTip(mojom::PublisherInfoPtr info,
                                         LegacyResultCallback callback) {
  WhenReady([this, info = std::move(info), callback]() mutable {
    database()->SavePublisherInfo(std::move(info), callback);
  });
}

void LedgerImpl::GetMonthlyReport(mojom::ActivityMonth month,
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

void LedgerImpl::ProcessSKU(const std::vector<mojom::SKUOrderItem>& items,
                            const std::string& wallet_type,
                            SKUOrderCallback callback) {
  WhenReady([this, items, wallet_type, callback]() {
    sku()->Process(items, wallet_type, callback);
  });
}

void LedgerImpl::Shutdown(LegacyResultCallback callback) {
  if (!IsReady()) {
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  ready_state_ = ReadyState::kShuttingDown;
  ledger_client_->ClearAllNotifications();

  wallet()->DisconnectAllWallets([this, callback](mojom::Result result) {
    BLOG_IF(1, result != mojom::Result::LEDGER_OK,
            "Not all wallets were disconnected");
    auto finish_callback = std::bind(&LedgerImpl::OnAllDone,
        this,
        _1,
        callback);
    database()->FinishAllInProgressContributions(finish_callback);
  });
}

void LedgerImpl::OnAllDone(mojom::Result result,
                           LegacyResultCallback callback) {
  database()->Close(callback);
}

void LedgerImpl::GetEventLogs(GetEventLogsCallback callback) {
  WhenReady([this, callback]() { database()->GetLastEventLogs(callback); });
}

bool LedgerImpl::IsShuttingDown() const {
  return ready_state_ == ReadyState::kShuttingDown;
}

void LedgerImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
  WhenReady([this, callback]() { callback(wallet()->GetWallet()); });
}

std::string LedgerImpl::GetRewardsWalletPassphrase() {
  if (!IsReady())
    return "";

  auto brave_wallet = wallet()->GetWallet();
  if (!brave_wallet) {
    return "";
  }

  return wallet()->GetWalletPassphrase(std::move(brave_wallet));
}

void LedgerImpl::LinkRewardsWallet(const std::string& destination_payment_id,
                                   PostSuggestionsClaimCallback callback) {
  WhenReady([this, destination_payment_id,
             callback = std::move(callback)]() mutable {
    wallet()->LinkRewardsWallet(destination_payment_id, std::move(callback));
  });
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

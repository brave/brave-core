/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_REWARDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_REWARDS_SERVICE_H_

#include "brave/components/brave_rewards/core/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include <cstdint>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/content/rewards_service.h"

namespace brave_ads::test {

// Minimal stub implementation of `brave_rewards::RewardsService` for unit
// tests. All methods are no-ops or immediately run their callbacks with safe
// default values. Only `GetRewardsWallet` matters for `AdsServiceImpl`
// initialization; it returns `nullptr` (no wallet) by default.
class FakeRewardsService : public brave_rewards::RewardsService {
 public:
  FakeRewardsService();

  FakeRewardsService(const FakeRewardsService&) = delete;
  FakeRewardsService& operator=(const FakeRewardsService&) = delete;

  ~FakeRewardsService() override;

  // brave_rewards::RewardsService:
  bool IsInitialized() override;
  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;
  std::string GetCountryCode() const override;
  void GetUserType(base::OnceCallback<void(brave_rewards::mojom::UserType)>
                       callback) override;
  bool IsTermsOfServiceUpdateRequired() override;
  void AcceptTermsOfServiceUpdate() override;
  void GetAvailableCountries(
      GetAvailableCountriesCallback callback) const override;
  void GetRewardsParameters(
      brave_rewards::GetRewardsParametersCallback callback) override;
  void FetchUICards(FetchUICardsCallback callback) override;
  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      brave_rewards::mojom::ActivityInfoFilterPtr filter,
      brave_rewards::GetPublisherInfoListCallback callback) override;
  void GetPublishersVisitedCount(
      base::OnceCallback<void(int)> callback) override;
  void GetExcludedList(
      brave_rewards::GetPublisherInfoListCallback callback) override;
  void RestorePublishers() override;
  void GetReconcileStamp(
      brave_rewards::GetReconcileStampCallback callback) override;
  void GetPublisherMinVisitTime(
      brave_rewards::GetPublisherMinVisitTimeCallback callback) override;
  void SetPublisherMinVisitTime(int duration_in_seconds) const override;
  void GetPublisherMinVisits(
      brave_rewards::GetPublisherMinVisitsCallback callback) override;
  void SetPublisherMinVisits(int visits) const override;
  void GetBalanceReport(
      uint32_t month,
      uint32_t year,
      brave_rewards::GetBalanceReportCallback callback) override;
  void NotifyPublisherPageVisit(
      brave_rewards::mojom::VisitDataPtr visit_data) override;
  void NotifyPublisherPageVisit(uint64_t tab_id,
                                const std::string& url) override;
  void GetPublisherBanner(
      const std::string& publisher_id,
      brave_rewards::GetPublisherBannerCallback callback) override;
  void OnTip(const std::string& publisher_key,
             double amount,
             bool recurring,
             brave_rewards::OnTipCallback callback) override;
  void RemoveRecurringTip(const std::string& publisher_key) override;
  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        base::OnceCallback<void(bool)> callback) override;
  void GetRecurringTips(
      brave_rewards::GetRecurringTipsCallback callback) override;
  void GetOneTimeTips(brave_rewards::GetOneTimeTipsCallback callback) override;
  void SetPublisherExclude(const std::string& publisher_key,
                           bool exclude) override;
  brave_rewards::RewardsNotificationService* GetNotificationService()
      const override;
  void GetRewardsInternalsInfo(
      brave_rewards::GetRewardsInternalsInfoCallback callback) override;
  void RefreshPublisher(
      const std::string& publisher_key,
      brave_rewards::RefreshPublisherCallback callback) override;
  void SaveRecurringTip(const std::string& publisher_key,
                        double amount,
                        brave_rewards::OnTipCallback callback) override;
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
  GetAllNotifications() override;
  void IsPublisherRegistered(const std::string& publisher_id,
                             base::OnceCallback<void(bool)> callback) override;
  void GetPublisherInfo(
      const std::string& publisher_key,
      brave_rewards::GetPublisherInfoCallback callback) override;
  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      brave_rewards::GetPublisherInfoCallback callback) override;
  void SavePublisherInfo(
      uint64_t window_id,
      brave_rewards::mojom::PublisherInfoPtr publisher_info,
      brave_rewards::SavePublisherInfoCallback callback) override;
  void GetShareURL(const base::flat_map<std::string, std::string>& args,
                   brave_rewards::GetShareURLCallback callback) override;
  void FetchBalance(brave_rewards::FetchBalanceCallback callback) override;
  void GetExternalWallet(
      brave_rewards::GetExternalWalletCallback callback) override;
  std::string GetExternalWalletType() const override;
  std::vector<std::string> GetExternalWalletProviders() const override;
  void BeginExternalWalletLogin(
      const std::string& wallet_type,
      BeginExternalWalletLoginCallback callback) override;
  void ConnectExternalWallet(
      const std::string& path,
      const std::string& query,
      brave_rewards::ConnectExternalWalletCallback callback) override;
  void ConnectExternalWallet(
      const std::string& provider,
      const base::flat_map<std::string, std::string>& args,
      brave_rewards::ConnectExternalWalletCallback callback) override;
  void GetAllContributions(
      brave_rewards::GetAllContributionsCallback callback) override;
  void WriteDiagnosticLog(const std::string& file,
                          int line,
                          int verbose_level,
                          const std::string& message) override;
  void LoadDiagnosticLog(
      int num_lines,
      brave_rewards::LoadDiagnosticLogCallback callback) override;
  void ClearDiagnosticLog(
      brave_rewards::ClearDiagnosticLogCallback callback) override;
  void CompleteReset(brave_rewards::SuccessCallback callback) override;
  void GetEventLogs(brave_rewards::GetEventLogsCallback callback) override;
  void GetRewardsWallet(
      brave_rewards::GetRewardsWalletCallback callback) override;
  void GetEnvironment(brave_rewards::GetEnvironmentCallback callback) override;
  brave_rewards::p3a::ConversionMonitor* GetP3AConversionMonitor() override;
  void OnRewardsPageShown() override;
};

}  // namespace brave_ads::test

#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_REWARDS_SERVICE_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_rewards_service.h"

#include <optional>
#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_rewards/content/rewards_notification_service.h"
#include "brave/components/brave_rewards/core/mojom/rewards.mojom.h"

namespace brave_ads::test {

FakeRewardsService::FakeRewardsService() = default;

FakeRewardsService::~FakeRewardsService() = default;

bool FakeRewardsService::IsInitialized() {
  return false;
}

void FakeRewardsService::CreateRewardsWallet(
    const std::string& /*country*/,
    CreateRewardsWalletCallback callback) {
  std::move(callback).Run(
      brave_rewards::mojom::CreateRewardsWalletResult::kSuccess);
}

std::string FakeRewardsService::GetCountryCode() const {
  return "";
}

void FakeRewardsService::GetUserType(
    base::OnceCallback<void(brave_rewards::mojom::UserType)> callback) {
  std::move(callback).Run(brave_rewards::mojom::UserType::kUnconnected);
}

bool FakeRewardsService::IsTermsOfServiceUpdateRequired() {
  return false;
}

void FakeRewardsService::AcceptTermsOfServiceUpdate() {}

void FakeRewardsService::GetAvailableCountries(
    GetAvailableCountriesCallback callback) const {
  std::move(callback).Run({});
}

void FakeRewardsService::GetRewardsParameters(
    brave_rewards::GetRewardsParametersCallback callback) {
  std::move(callback).Run(nullptr);
}

void FakeRewardsService::FetchUICards(FetchUICardsCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void FakeRewardsService::GetActivityInfoList(
    uint32_t /*start*/,
    uint32_t /*limit*/,
    brave_rewards::mojom::ActivityInfoFilterPtr /*filter*/,
    brave_rewards::GetPublisherInfoListCallback callback) {
  std::move(callback).Run({});
}

void FakeRewardsService::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  std::move(callback).Run(0);
}

void FakeRewardsService::GetExcludedList(
    brave_rewards::GetPublisherInfoListCallback callback) {
  std::move(callback).Run({});
}

void FakeRewardsService::RestorePublishers() {}

void FakeRewardsService::GetReconcileStamp(
    brave_rewards::GetReconcileStampCallback callback) {
  std::move(callback).Run(0);
}

void FakeRewardsService::GetPublisherMinVisitTime(
    brave_rewards::GetPublisherMinVisitTimeCallback callback) {
  std::move(callback).Run(0);
}

void FakeRewardsService::SetPublisherMinVisitTime(
    int /*duration_in_seconds*/) const {}

void FakeRewardsService::GetPublisherMinVisits(
    brave_rewards::GetPublisherMinVisitsCallback callback) {
  std::move(callback).Run(0);
}

void FakeRewardsService::SetPublisherMinVisits(int /*visits*/) const {}

void FakeRewardsService::GetBalanceReport(
    uint32_t /*month*/,
    uint32_t /*year*/,
    brave_rewards::GetBalanceReportCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Result::FAILED, nullptr);
}

void FakeRewardsService::NotifyPublisherPageVisit(
    brave_rewards::mojom::VisitDataPtr /*visit_data*/) {}

void FakeRewardsService::NotifyPublisherPageVisit(uint64_t /*tab_id*/,
                                                  const std::string& /*url*/) {}

void FakeRewardsService::GetPublisherBanner(
    const std::string& /*publisher_id*/,
    brave_rewards::GetPublisherBannerCallback callback) {
  std::move(callback).Run(nullptr);
}

void FakeRewardsService::OnTip(const std::string& /*publisher_key*/,
                               double /*amount*/,
                               bool /*recurring*/,
                               brave_rewards::OnTipCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Result::FAILED);
}

void FakeRewardsService::RemoveRecurringTip(
    const std::string& /*publisher_key*/) {}

void FakeRewardsService::SendContribution(
    const std::string& /*publisher_id*/,
    double /*amount*/,
    bool /*set_monthly*/,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(false);
}

void FakeRewardsService::GetRecurringTips(
    brave_rewards::GetRecurringTipsCallback callback) {
  std::move(callback).Run({});
}

void FakeRewardsService::GetOneTimeTips(
    brave_rewards::GetOneTimeTipsCallback callback) {
  std::move(callback).Run({});
}

void FakeRewardsService::SetPublisherExclude(
    const std::string& /*publisher_key*/,
    bool /*exclude*/) {}

brave_rewards::RewardsNotificationService*
FakeRewardsService::GetNotificationService() const {
  return nullptr;
}

void FakeRewardsService::GetRewardsInternalsInfo(
    brave_rewards::GetRewardsInternalsInfoCallback callback) {
  std::move(callback).Run(nullptr);
}

void FakeRewardsService::RefreshPublisher(
    const std::string& /*publisher_key*/,
    brave_rewards::RefreshPublisherCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::PublisherStatus::NOT_VERIFIED,
                          "");
}

void FakeRewardsService::SaveRecurringTip(
    const std::string& /*publisher_key*/,
    double /*amount*/,
    brave_rewards::OnTipCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Result::FAILED);
}

const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
FakeRewardsService::GetAllNotifications() {
  static const base::NoDestructor<
      brave_rewards::RewardsNotificationService::RewardsNotificationsMap>
      kEmpty;
  return *kEmpty;
}

void FakeRewardsService::IsPublisherRegistered(
    const std::string& /*publisher_id*/,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(false);
}

void FakeRewardsService::GetPublisherInfo(
    const std::string& /*publisher_key*/,
    brave_rewards::GetPublisherInfoCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Result::FAILED, nullptr);
}

void FakeRewardsService::GetPublisherPanelInfo(
    const std::string& /*publisher_key*/,
    brave_rewards::GetPublisherInfoCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Result::FAILED, nullptr);
}

void FakeRewardsService::SavePublisherInfo(
    uint64_t /*window_id*/,
    brave_rewards::mojom::PublisherInfoPtr /*publisher_info*/,
    brave_rewards::SavePublisherInfoCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Result::FAILED);
}

void FakeRewardsService::GetShareURL(
    const base::flat_map<std::string, std::string>& /*args*/,
    brave_rewards::GetShareURLCallback callback) {
  std::move(callback).Run("");
}

void FakeRewardsService::FetchBalance(
    brave_rewards::FetchBalanceCallback callback) {
  std::move(callback).Run(nullptr);
}

void FakeRewardsService::GetExternalWallet(
    brave_rewards::GetExternalWalletCallback callback) {
  std::move(callback).Run(nullptr);
}

std::string FakeRewardsService::GetExternalWalletType() const {
  return "";
}

std::vector<std::string> FakeRewardsService::GetExternalWalletProviders()
    const {
  return {};
}

void FakeRewardsService::BeginExternalWalletLogin(
    const std::string& /*wallet_type*/,
    BeginExternalWalletLoginCallback callback) {
  std::move(callback).Run(nullptr);
}

void FakeRewardsService::ConnectExternalWallet(
    const std::string& /*path*/,
    const std::string& /*query*/,
    brave_rewards::ConnectExternalWalletCallback callback) {
  std::move(callback).Run(
      brave_rewards::mojom::ConnectExternalWalletResult::kUnexpected);
}

void FakeRewardsService::ConnectExternalWallet(
    const std::string& /*provider*/,
    const base::flat_map<std::string, std::string>& /*args*/,
    brave_rewards::ConnectExternalWalletCallback callback) {
  std::move(callback).Run(
      brave_rewards::mojom::ConnectExternalWalletResult::kUnexpected);
}

void FakeRewardsService::GetAllContributions(
    brave_rewards::GetAllContributionsCallback callback) {
  std::move(callback).Run({});
}

void FakeRewardsService::WriteDiagnosticLog(const std::string& /*file*/,
                                            int /*line*/,
                                            int /*verbose_level*/,
                                            const std::string& /*message*/) {}

void FakeRewardsService::LoadDiagnosticLog(
    int /*num_lines*/,
    brave_rewards::LoadDiagnosticLogCallback callback) {
  std::move(callback).Run("");
}

void FakeRewardsService::ClearDiagnosticLog(
    brave_rewards::ClearDiagnosticLogCallback callback) {
  std::move(callback).Run(true);
}

void FakeRewardsService::CompleteReset(
    brave_rewards::SuccessCallback callback) {
  std::move(callback).Run(false);
}

void FakeRewardsService::GetEventLogs(
    brave_rewards::GetEventLogsCallback callback) {
  std::move(callback).Run({});
}

void FakeRewardsService::GetRewardsWallet(
    brave_rewards::GetRewardsWalletCallback callback) {
  std::move(callback).Run(nullptr);
}

void FakeRewardsService::GetEnvironment(
    brave_rewards::GetEnvironmentCallback callback) {
  std::move(callback).Run(brave_rewards::mojom::Environment::kProduction);
}

brave_rewards::p3a::ConversionMonitor*
FakeRewardsService::GetP3AConversionMonitor() {
  return nullptr;
}

void FakeRewardsService::OnRewardsPageShown() {}

}  // namespace brave_ads::test

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IMPL_MOCK_H_
#define BAT_LEDGER_LEDGER_IMPL_MOCK_H_

#include <stdint.h>

#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "bat/ledger/ledger_client.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace bat_ledger {

class MockLogStreamImpl : public ledger::LogStream {
 public:
  MockLogStreamImpl(
      const char* file,
      const int line,
      const ledger::LogLevel log_level);

  std::ostream& stream() override;

 private:
  // Not copyable, not assignable
  MockLogStreamImpl(const MockLogStreamImpl&) = delete;
  MockLogStreamImpl& operator=(const MockLogStreamImpl&) = delete;
};

class MockLedgerImpl : public LedgerImpl {
 public:
  explicit MockLedgerImpl(ledger::LedgerClient* client);

  ~MockLedgerImpl() override;

  std::unique_ptr<ledger::LogStream> Log(
      const char* file,
      const int line,
      const ledger::LogLevel log_level) const;

  MOCK_METHOD2(Initialize, void(
      const bool,
      ledger::ResultCallback));

  MOCK_METHOD1(CreateWallet, void(ledger::ResultCallback));

  MOCK_METHOD2(SavePublisherInfo, void(
      ledger::PublisherInfoPtr,
      ledger::ResultCallback));

  MOCK_METHOD2(SaveActivityInfo, void(
      ledger::PublisherInfoPtr,
      ledger::ResultCallback));

  MOCK_METHOD2(GetPublisherInfo,
      void(const std::string&, ledger::PublisherInfoCallback));

  MOCK_METHOD2(GetActivityInfo,
      void(ledger::ActivityInfoFilterPtr, ledger::PublisherInfoCallback));

  MOCK_METHOD2(GetPanelPublisherInfo,
      void(ledger::ActivityInfoFilterPtr, ledger::PublisherInfoCallback));

  MOCK_METHOD2(GetMediaPublisherInfo,
      void(const std::string&, ledger::PublisherInfoCallback));

  MOCK_METHOD3(SaveMediaPublisherInfo,
      void(const std::string&,
          const std::string&,
          ledger::ResultCallback));

  MOCK_METHOD4(GetActivityInfoList,
      void(uint32_t,
          uint32_t,
          ledger::ActivityInfoFilterPtr,
          ledger::PublisherInfoListCallback));

  MOCK_METHOD3(OneTimeTip, void(
      const std::string&,
      const double,
      ledger::ResultCallback));

  MOCK_METHOD1(SetRewardsMainEnabled, void(bool));

  MOCK_METHOD1(SetPublisherMinVisitTime, void(uint64_t));

  MOCK_METHOD1(SetPublisherMinVisits, void(unsigned int));

  MOCK_METHOD1(SetPublisherAllowNonVerified, void(bool));

  MOCK_METHOD1(SetPublisherAllowVideos, void(bool));

  MOCK_METHOD1(SetContributionAmount, void(double));

  MOCK_METHOD0(SetUserChangedContribution, void());

  MOCK_METHOD0(GetUserChangedContribution, bool());

  MOCK_METHOD1(SetAutoContribute, void(bool));

  MOCK_METHOD0(UpdateAdsRewards, void());

  MOCK_METHOD2(SavePendingContribution,
      void(ledger::PendingContributionList,
          ledger::ResultCallback));

  MOCK_CONST_METHOD0(GetReconcileStamp, uint64_t());

  MOCK_CONST_METHOD0(GetRewardsMainEnabled, bool());

  MOCK_CONST_METHOD0(GetPublisherMinVisitTime, uint64_t());

  MOCK_CONST_METHOD0(GetPublisherMinVisits, unsigned int());

  MOCK_CONST_METHOD0(GetPublisherAllowNonVerified, bool());

  MOCK_CONST_METHOD0(GetPublisherAllowVideos, bool());

  MOCK_CONST_METHOD0(GetContributionAmount, double());

  MOCK_CONST_METHOD0(GetAutoContribute, bool());

  MOCK_CONST_METHOD3(GetBalanceReport,
      void(const ledger::ActivityMonth,
          const int,
          const ledger::GetBalanceReportCallback));

  MOCK_CONST_METHOD0(GetAllBalanceReports,
      std::map<std::string, ledger::BalanceReportInfoPtr>());

  MOCK_METHOD0(GetAutoContributeProps, ledger::AutoContributePropsPtr());

  MOCK_METHOD1(SaveLedgerState, void(const std::string&));

  MOCK_METHOD2(SavePublisherState, void(
      const std::string&,
      ledger::ResultCallback));

  MOCK_METHOD1(LoadNicewareList, void(ledger::GetNicewareListCallback));

  MOCK_METHOD1(SetConfirmationsWalletInfo,
      void(const ledger::WalletInfoProperties&));

  MOCK_METHOD1(LoadLedgerState, void(ledger::OnLoadCallback));

  MOCK_METHOD1(LoadPublisherState, void(ledger::OnLoadCallback));

  MOCK_METHOD2(OnWalletInitializedInternal,
      void(ledger::Result, ledger::ResultCallback));

  MOCK_METHOD2(OnWalletProperties,
      void(ledger::Result,
          const ledger::WalletProperties&));

  MOCK_CONST_METHOD1(FetchWalletProperties,
      void(const ledger::OnWalletPropertiesCallback));

  MOCK_CONST_METHOD1(FetchPromotions,
      void(ledger::FetchPromotionCallback));

  MOCK_CONST_METHOD2(ClaimPromotion,
      void(const std::string&, const ledger::ClaimPromotionCallback));

  MOCK_CONST_METHOD3(AttestPromotion,
      void(const std::string&,
          const std::string&,
          const ledger::AttestPromotionCallback));

  MOCK_CONST_METHOD0(GetWalletPassphrase, std::string());

  MOCK_METHOD2(RecoverWallet,
      void(const std::string&, ledger::RecoverWalletCallback));

  MOCK_METHOD3(OnRecoverWallet,
      void(const ledger::Result, double, ledger::RecoverWalletCallback));

  MOCK_METHOD6(LoadURL,
      void(const std::string&,
          const std::vector<std::string>&,
          const std::string&,
          const std::string&,
          const ledger::UrlMethod,
          ledger::LoadURLCallback));

  MOCK_METHOD4(ContributionCompleted, void(
      const ledger::Result,
      const double,
      const std::string&,
      const ledger::RewardsType));

  MOCK_METHOD1(URIEncode, std::string(const std::string&));

  MOCK_METHOD5(SaveMediaVisit,
      void(const std::string&,
          const ledger::VisitData&,
          const uint64_t&,
          const uint64_t,
          const ledger::PublisherInfoCallback));

  MOCK_METHOD3(SetPublisherExclude,
      void(const std::string&,
          const ledger::PublisherExclude&,
          ledger::ResultCallback));

  MOCK_METHOD1(RestorePublishers, void(ledger::ResultCallback));

  MOCK_METHOD2(OnRestorePublishers,
      void(const ledger::Result, ledger::ResultCallback));

  MOCK_CONST_METHOD0(IsWalletCreated, bool());

  MOCK_METHOD3(GetPublisherActivityFromUrl,
      void(uint64_t,
          ledger::VisitDataPtr,
          const std::string&));

  MOCK_METHOD4(GetMediaActivityFromUrl,
      void(uint64_t,
          ledger::VisitDataPtr,
          const std::string&,
          const std::string&));

  MOCK_METHOD3(OnPanelPublisherInfo,
      void(ledger::Result, ledger::PublisherInfoPtr, uint64_t));

  MOCK_METHOD4(SetBalanceReportItem, void(
      const ledger::ActivityMonth,
      const int,
      const ledger::ReportType,
      const double));

  MOCK_METHOD3(FetchFavIcon,
      void(const std::string&,
          const std::string&,
          ledger::FetchIconCallback));

  MOCK_METHOD2(GetPublisherBanner,
      void(const std::string&, ledger::PublisherBannerCallback));

  MOCK_METHOD2(SaveRecurringTip,
      void(ledger::RecurringTipPtr, ledger::ResultCallback));

  MOCK_METHOD1(GetRecurringTips, void(ledger::PublisherInfoListCallback));

  MOCK_METHOD1(GetOneTimeTips, void(ledger::PublisherInfoListCallback));

  MOCK_METHOD2(RemoveRecurringTip,
      void(const std::string&, ledger::ResultCallback));

  MOCK_METHOD6(CreateActivityFilter,
      ledger::ActivityInfoFilterPtr(const std::string&,
          ledger::ExcludeFilter,
          bool,
          const uint64_t&,
          bool,
          bool));

  MOCK_METHOD4(LogResponse,
      void(const std::string&,
          int,
          const std::string&,
          const std::map<std::string, std::string>&));

  MOCK_METHOD0(ResetReconcileStamp, void());

  MOCK_CONST_METHOD0(GetPaymentId, const std::string&());

  MOCK_CONST_METHOD0(GetPersonaId, const std::string&());

  MOCK_METHOD1(SetPersonaId, void(const std::string&));

  MOCK_CONST_METHOD0(GetUserId, const std::string&());

  MOCK_METHOD1(SetUserId, void(const std::string&));

  MOCK_CONST_METHOD0(GetRegistrarVK, const std::string&());

  MOCK_METHOD1(SetRegistrarVK, void(const std::string&));

  MOCK_CONST_METHOD0(GetPreFlight, const std::string&());

  MOCK_METHOD1(SetPreFlight, void(const std::string&));

  MOCK_CONST_METHOD0(GetWalletInfo,
      const ledger::WalletInfoProperties&());

  MOCK_METHOD1(SetWalletInfo,
      void(const ledger::WalletInfoProperties&));

  MOCK_METHOD1(GetConfirmationsWalletInfo,
      const confirmations::WalletInfo(
          const ledger::WalletInfoProperties&));

  MOCK_CONST_METHOD0(GetWalletProperties,
      const ledger::WalletProperties&());

  MOCK_METHOD1(SetWalletProperties,
      void(ledger::WalletProperties*));

  MOCK_CONST_METHOD0(GetBootStamp, uint64_t());

  MOCK_METHOD1(SetBootStamp, void(uint64_t));

  MOCK_METHOD6(SaveContributionInfo,
      void(const std::string&,
          const ledger::ActivityMonth,
          const int,
          const uint32_t,
          const std::string&,
          const ledger::RewardsType));

  MOCK_METHOD3(NormalizeContributeWinners,
      void(ledger::PublisherInfoList*,
          const ledger::PublisherInfoList*,
          uint32_t));

  MOCK_CONST_METHOD2(SetTimer, void(uint64_t, uint32_t*));

  MOCK_METHOD0(GetDefaultContributionAmount, double());

  MOCK_METHOD1(HasSufficientBalanceToReconcile,
      void(ledger::HasSufficientBalanceToReconcileCallback));

  MOCK_METHOD1(SaveNormalizedPublisherList, void(ledger::PublisherInfoList));

  MOCK_METHOD1(SetCatalogIssuers, void(
      const std::string&));

  MOCK_METHOD2(ConfirmAd, void(
      const std::string&,
      const std::string&));

  MOCK_METHOD3(ConfirmAction, void(
      const std::string&,
      const std::string&,
      const std::string&));

  MOCK_METHOD1(GetTransactionHistory,
      void(ledger::GetTransactionHistoryCallback));

  MOCK_METHOD0(GetTaskRunner, scoped_refptr<base::SequencedTaskRunner>());

  MOCK_METHOD1(GetRewardsInternalsInfo,
      void(ledger::RewardsInternalsInfoCallback));

  MOCK_METHOD0(StartMonthlyContribution, void());

  MOCK_METHOD2(RefreshPublisher,
      void(const std::string&, ledger::OnRefreshPublisherCallback));

  MOCK_METHOD3(SaveMediaInfo,
      void(const std::string&,
          const std::map<std::string, std::string>&,
          ledger::PublisherInfoCallback));

  MOCK_METHOD2(SetInlineTipSetting, void(const std::string&, bool));

  MOCK_METHOD1(GetInlineTipSetting, bool(const std::string&));

  MOCK_METHOD2(GetShareURL,
      std::string(const std::string&,
          const std::map<std::string, std::string>&));

  MOCK_METHOD1(GetPendingContributions,
      void(ledger::PendingContributionInfoListCallback));

  MOCK_METHOD2(RemovePendingContribution,
      void(const uint64_t id,
          ledger::ResultCallback));

  MOCK_METHOD1(RemoveAllPendingContributions,
      void(ledger::ResultCallback));

  MOCK_METHOD1(GetPendingContributionsTotal,
      void(ledger::PendingContributionsTotalCallback));

  MOCK_METHOD0(ContributeUnverifiedPublishers, void());

  MOCK_METHOD3(OnContributeUnverifiedPublishers,
      void(ledger::Result, const std::string&, const std::string&));

  MOCK_METHOD1(SavePublisherProcessed, void(const std::string&));

  MOCK_CONST_METHOD1(WasPublisherAlreadyProcessed, bool(const std::string&));

  MOCK_METHOD1(FetchBalance, void(ledger::FetchBalanceCallback));

  MOCK_METHOD1(GetExternalWallets, void(ledger::GetExternalWalletsCallback));

  MOCK_CONST_METHOD0(GetCardIdAddress, std::string());

  MOCK_METHOD2(GetExternalWallet,
      void(const std::string&, ledger::ExternalWalletCallback));

  MOCK_METHOD2(SaveExternalWallet,
      void(const std::string&, ledger::ExternalWalletPtr));

  MOCK_METHOD3(ExternalWalletAuthorization,
      void(const std::string&,
          const std::map<std::string, std::string>&,
          ledger::ExternalWalletAuthorizationCallback));

  MOCK_METHOD2(DisconnectWallet,
      void(const std::string&, ledger::ResultCallback));

  MOCK_METHOD3(TransferAnonToExternalWallet,
      void(ledger::ExternalWalletPtr,
          ledger::ResultCallback,
          const bool));

  MOCK_METHOD3(ShowNotification,
      void(const std::string&,
          ledger::ResultCallback,
          const std::vector<std::string>&));

  MOCK_METHOD2(DeleteActivityInfo,
      void(const std::string&, ledger::ResultCallback));

  MOCK_METHOD1(ClearServerPublisherList, void(ledger::ResultCallback));

  MOCK_METHOD2(InsertServerPublisherList, void(
      const std::vector<ledger::ServerPublisherPartial>&,
      ledger::ResultCallback));

  MOCK_METHOD2(InsertPublisherBannerList, void(
      const std::vector<ledger::PublisherBanner>&,
      ledger::ResultCallback));

  MOCK_METHOD2(GetServerPublisherInfo, void(
      const std::string&,
      ledger::GetServerPublisherInfoCallback));

  MOCK_METHOD1(IsPublisherConnectedOrVerified,
      bool(const ledger::PublisherStatus));

  MOCK_METHOD2(SetBooleanState, void(const std::string&, bool));

  MOCK_CONST_METHOD1(GetBooleanState, bool(const std::string&));

  MOCK_METHOD2(SetIntegerState, void(const std::string&, int));

  MOCK_CONST_METHOD1(GetIntegerState, int(const std::string&));

  MOCK_METHOD2(SetDoubleState, void(const std::string&, double));

  MOCK_CONST_METHOD1(GetDoubleState, double(const std::string&));

  MOCK_METHOD2(SetStringState, void(const std::string&, const std::string&));

  MOCK_CONST_METHOD1(GetStringState, std::string(const std::string&));

  MOCK_METHOD2(SetInt64State, void(const std::string&, int64_t));

  MOCK_CONST_METHOD1(GetInt64State, int64_t(const std::string&));

  MOCK_METHOD2(SetUint64State, void(const std::string&, uint64_t));

  MOCK_CONST_METHOD1(GetUint64State, uint64_t(const std::string&));

  MOCK_METHOD1(ClearState, void(const std::string&));

  MOCK_CONST_METHOD1(GetBooleanOption, bool(const std::string&));

  MOCK_CONST_METHOD1(GetIntegerOption, int(const std::string&));

  MOCK_CONST_METHOD1(GetDoubleOption, double(const std::string&));

  MOCK_CONST_METHOD1(GetStringOption, std::string(const std::string&));

  MOCK_CONST_METHOD1(GetInt64Option, int64_t(const std::string&));

  MOCK_CONST_METHOD1(GetUint64Option, uint64_t(const std::string&));

  MOCK_METHOD2(SetTransferFee,
      void(const std::string&, ledger::TransferFeePtr));

  MOCK_CONST_METHOD1(GetTransferFees,
      ledger::TransferFeeList(const std::string&));

  MOCK_METHOD2(RemoveTransferFee, void(const std::string&, const std::string&));

  MOCK_METHOD2(SaveContributionQueue,
      void(ledger::ContributionQueuePtr, ledger::ResultCallback));

  MOCK_METHOD2(DeleteContributionQueue,
      void(const uint64_t, ledger::ResultCallback));

  MOCK_METHOD1(GetFirstContributionQueue,
      void(ledger::GetFirstContributionQueueCallback));

  MOCK_METHOD2(SavePromotion,
      void(ledger::PromotionPtr, ledger::ResultCallback));

  MOCK_METHOD2(GetPromotion,
      void(const std::string&, ledger::GetPromotionCallback));

  MOCK_METHOD1(GetAllPromotions, void(ledger::GetAllPromotionsCallback));

  MOCK_METHOD2(DeletePromotionList, void(
      const std::vector<std::string>&,
      ledger::ResultCallback));

  MOCK_METHOD2(SaveUnblindedTokenList, void(
    ledger::UnblindedTokenList, ledger::ResultCallback));

  MOCK_METHOD1(GetAllUnblindedTokens,
      void(ledger::GetUnblindedTokenListCallback));

  MOCK_METHOD2(DeleteUnblindedTokens,
      void(const std::vector<std::string>&, ledger::ResultCallback));

  MOCK_METHOD0(GetClientInfo, ledger::ClientInfoPtr());

  MOCK_METHOD0(UnblindedTokensReady, void());

  MOCK_METHOD1(GetAnonWalletStatus, void(ledger::ResultCallback));

  MOCK_METHOD2(GetUnblindedTokensByTriggerIds, void(
      const std::vector<std::string>& trigger_ids,
      ledger::GetUnblindedTokenListCallback));

  MOCK_METHOD2(GetContributionInfo, void(
      const std::string& contribution_id,
      ledger::GetContributionInfoCallback callback));

  MOCK_METHOD3(UpdateContributionInfoStep, void(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      ledger::ResultCallback callback));

  MOCK_METHOD4(UpdateContributionInfoStepAndCount, void(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback));

  MOCK_METHOD2(RunDBTransaction, void(
      ledger::DBTransactionPtr,
      ledger::RunDBTransactionCallback));

  MOCK_METHOD1(GetCreateScript, void(ledger::GetCreateScriptCallback callback));
};

}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_MOCK_H_

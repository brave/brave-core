/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IMPL_MOCK_H_
#define BAT_LEDGER_LEDGER_IMPL_MOCK_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger_client.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/prefix_list_reader.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace bat_ledger {

class MockLedgerImpl : public LedgerImpl {
 public:
  explicit MockLedgerImpl(ledger::LedgerClient* client);

  ~MockLedgerImpl() override;

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

  MOCK_METHOD1(SetPublisherMinVisitTime, void(int));

  MOCK_METHOD1(SetPublisherMinVisits, void(int));

  MOCK_METHOD1(SetPublisherAllowNonVerified, void(bool));

  MOCK_METHOD1(SetPublisherAllowVideos, void(bool));

  MOCK_METHOD1(SetAutoContributionAmount, void(double));

  MOCK_METHOD1(SetAutoContributeEnabled, void(bool));

  MOCK_METHOD2(SavePendingContribution,
      void(ledger::PendingContributionList,
          ledger::ResultCallback));

  MOCK_METHOD0(GetReconcileStamp, uint64_t());

  MOCK_METHOD0(GetRewardsMainEnabled, bool());

  MOCK_METHOD0(GetPublisherMinVisitTime, int());

  MOCK_METHOD0(GetPublisherMinVisits, int());

  MOCK_METHOD0(GetPublisherAllowNonVerified, bool());

  MOCK_METHOD0(GetPublisherAllowVideos, bool());

  MOCK_METHOD0(GetAutoContributionAmount, double());

  MOCK_METHOD0(GetAutoContributeEnabled, bool());

  MOCK_CONST_METHOD3(GetBalanceReport,
      void(const ledger::ActivityMonth,
          const int,
          const ledger::GetBalanceReportCallback));

  MOCK_CONST_METHOD1(GetAllBalanceReports,
      void(ledger::GetBalanceReportListCallback));

  MOCK_METHOD0(GetAutoContributeProperties,
      ledger::AutoContributePropertiesPtr());

  MOCK_METHOD1(LoadLedgerState, void(ledger::OnLoadCallback));

  MOCK_METHOD1(LoadPublisherState, void(ledger::OnLoadCallback));

  MOCK_METHOD2(OnWalletInitializedInternal,
      void(ledger::Result, ledger::ResultCallback));

  MOCK_METHOD1(GetRewardsParameters,
      void(const ledger::GetRewardsParametersCallback));

  MOCK_CONST_METHOD1(FetchPromotions,
      void(ledger::FetchPromotionCallback));

  MOCK_CONST_METHOD3(ClaimPromotion, void(
      const std::string&,
      const std::string&,
      const ledger::ClaimPromotionCallback));

  MOCK_CONST_METHOD3(AttestPromotion,
      void(const std::string&,
          const std::string&,
          const ledger::AttestPromotionCallback));

  MOCK_CONST_METHOD0(GetWalletPassphrase, std::string());

  MOCK_METHOD2(RecoverWallet,
      void(const std::string&, ledger::ResultCallback));

  MOCK_METHOD6(LoadURL,
      void(const std::string&,
          const std::vector<std::string>&,
          const std::string&,
          const std::string&,
          const ledger::UrlMethod,
          ledger::LoadURLCallback));

  MOCK_METHOD2(ContributionCompleted, void(
      const ledger::Result,
      ledger::ContributionInfoPtr contribution));

  MOCK_METHOD5(SaveVisit,
      void(const std::string&,
          const ledger::VisitData&,
          uint64_t,
          uint64_t,
          ledger::PublisherInfoCallback));

  MOCK_METHOD5(SaveVideoVisit,
      void(const std::string&,
          const ledger::VisitData&,
          uint64_t,
          uint64_t,
          ledger::PublisherInfoCallback));

  MOCK_METHOD3(SetPublisherExclude,
      void(const std::string&,
          const ledger::PublisherExclude&,
          ledger::ResultCallback));

  MOCK_METHOD1(RestorePublishers, void(ledger::ResultCallback));

  MOCK_METHOD2(OnRestorePublishers,
      void(const ledger::Result, ledger::ResultCallback));

  MOCK_METHOD0(IsWalletCreated, bool());

  MOCK_METHOD3(GetPublisherActivityFromUrl,
      void(uint64_t,
          ledger::VisitDataPtr,
          const std::string&));

  MOCK_METHOD4(GetMediaActivityFromUrl,
      void(uint64_t,
          ledger::VisitDataPtr,
          const std::string&,
          const std::string&));

  MOCK_METHOD4(SetBalanceReportItem, void(
      const ledger::ActivityMonth,
      const int,
      const ledger::ReportType,
      const double));

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

  MOCK_METHOD0(ResetReconcileStamp, void());

  MOCK_METHOD0(GetCreationStamp, uint64_t());

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

  MOCK_METHOD1(HasSufficientBalanceToReconcile,
      void(ledger::HasSufficientBalanceToReconcileCallback));

  MOCK_METHOD1(SaveNormalizedPublisherList, void(ledger::PublisherInfoList));

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

  MOCK_METHOD2(SetInlineTippingPlatformEnabled,
      void(const ledger::InlineTipsPlatforms platform, bool));

  MOCK_METHOD1(GetInlineTippingPlatformEnabled,
      bool(const ledger::InlineTipsPlatforms platform));

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

  MOCK_METHOD2(WasPublisherProcessed, void(
      const std::string&,
      ledger::ResultCallback));

  MOCK_METHOD1(FetchBalance, void(ledger::FetchBalanceCallback));

  MOCK_METHOD2(GetExternalWallet,
      void(const std::string&, ledger::ExternalWalletCallback));

  MOCK_METHOD3(ExternalWalletAuthorization,
      void(const std::string&,
          const std::map<std::string, std::string>&,
          ledger::ExternalWalletAuthorizationCallback));

  MOCK_METHOD2(DisconnectWallet,
      void(const std::string&, ledger::ResultCallback));

  MOCK_METHOD1(ClaimFunds, void(ledger::ResultCallback));

  MOCK_METHOD2(DeleteActivityInfo,
      void(const std::string&, ledger::ResultCallback));

  MOCK_METHOD2(SearchPublisherPrefixList,
      void(const std::string&, ledger::SearchPublisherPrefixListCallback));

  MOCK_METHOD2(ResetPublisherPrefixList, void(
      std::unique_ptr<braveledger_publisher::PrefixListReader>,
      ledger::ResultCallback));

  MOCK_METHOD2(InsertServerPublisherInfo,
      void(const ledger::ServerPublisherInfo&, ledger::ResultCallback));

  MOCK_METHOD2(GetServerPublisherInfo, void(
      const std::string&,
      ledger::GetServerPublisherInfoCallback));

  MOCK_METHOD1(IsPublisherConnectedOrVerified,
      bool(const ledger::PublisherStatus));

  MOCK_METHOD2(SaveContributionQueue,
      void(ledger::ContributionQueuePtr, ledger::ResultCallback));

  MOCK_METHOD2(MarkContributionQueueAsComplete,
      void(const uint64_t, ledger::ResultCallback));

  MOCK_METHOD1(GetFirstContributionQueue,
      void(ledger::GetFirstContributionQueueCallback));

  MOCK_METHOD2(SavePromotion,
      void(ledger::PromotionPtr, ledger::ResultCallback));

  MOCK_METHOD2(GetPromotion,
      void(const std::string&, ledger::GetPromotionCallback));

  MOCK_METHOD1(GetAllPromotions, void(ledger::GetAllPromotionsCallback));

  MOCK_METHOD2(SaveUnblindedTokenList, void(
    ledger::UnblindedTokenList, ledger::ResultCallback));

  MOCK_METHOD4(MarkUnblindedTokensAsSpent, void(
      const std::vector<std::string>&,
      ledger::RewardsType,
      const std::string&,
      ledger::ResultCallback));

  MOCK_METHOD1(GetAnonWalletStatus, void(ledger::ResultCallback));

  MOCK_METHOD2(GetSpendableUnblindedTokensByTriggerIds, void(
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

  MOCK_METHOD2(GetReservedUnblindedTokens, void(
      const std::string&,
      ledger::GetUnblindedTokenListCallback));

  MOCK_METHOD2(GetSpendableUnblindedTokensByBatchTypes, void(
      const std::vector<ledger::CredsBatchType>&,
      ledger::GetUnblindedTokenListCallback));
};

}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_MOCK_H_

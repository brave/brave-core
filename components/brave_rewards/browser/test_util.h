/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_UTIL_H_

#include <memory>

#include "base/files/file_path.h"

class KeyedService;
class Profile;

namespace brave_rewards {

class MockLedgerClient : public ledger::Ledger {
 public:
  MockLedgerClient();
  ~MockLedgerClient() override;

  MOCK_METHOD2(OnLoad, void(const ledger::VisitData& visit_data,
                            const uint64_t& current_time));

  MOCK_METHOD0(Initialize, void());

  MOCK_METHOD0(CreateWallet, bool());

  MOCK_METHOD1(MakePayment, void(const ledger::PaymentData& payment_data));

  MOCK_METHOD2(AddRecurringPayment, void(const std::string& publisher_id,
                                         const double& value));

  MOCK_METHOD3(DoDirectDonation, void(const ledger::PublisherInfo& publisher,
                                      const int amount,
                                      const std::string& currency));

  MOCK_METHOD2(OnUnload, void(uint32_t tab_id, const uint64_t& current_time));

  MOCK_METHOD2(OnShow, void(uint32_t tab_id, const uint64_t& current_time));

  MOCK_METHOD2(OnHide, void(uint32_t tab_id, const uint64_t& current_time));

  MOCK_METHOD2(OnForeground, void(uint32_t tab_id,
                                  const uint64_t& current_time));

  MOCK_METHOD2(OnBackground, void(uint32_t tab_id,
                                  const uint64_t& current_time));

  MOCK_METHOD2(OnMediaStart, void(uint32_t tab_id,
                                  const uint64_t& current_time));

  MOCK_METHOD2(OnMediaStop, void(uint32_t tab_id,
                                 const uint64_t& current_time));
  MOCK_METHOD6(OnXHRLoad, void(
      uint32_t tab_id,
      const std::string& url,
      const std::map<std::string, std::string>& parts,
      const std::string& first_party_url,
      const std::string& referrer,
      const ledger::VisitData& visit_data));

  MOCK_METHOD5(OnPostData, void(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      const ledger::VisitData& visit_data));

  MOCK_METHOD1(OnTimer, void(uint32_t timer_id));

  MOCK_METHOD1(URIEncode, std::string(const std::string& value));

  MOCK_METHOD2(SetPublisherInfo,
      void(std::unique_ptr<ledger::PublisherInfo> publisher_info,
           ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(SetActivityInfo,
      void(std::unique_ptr<ledger::PublisherInfo> publisher_info,
           ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(GetPublisherInfo, void(const std::string& publisher_key,
                                      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(GetActivityInfo, void(const ledger::ActivityInfoFilter& filter,
                                      ledger::PublisherInfoCallback callback));
 
  MOCK_METHOD2(SetMediaPublisherInfo, void(const std::string& media_key,
                                           const std::string& publisher_id));

  MOCK_METHOD2(GetMediaPublisherInfo, void(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD4(GetActivityInfoList, void(
      uint32_t start,
      uint32_t limit,
      const ledger::ActivityInfoFilter &filter,
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD0(GetRecurringDonationPublisherInfo,
               std::vector<ledger::ContributionInfo>());

  MOCK_METHOD4(GetPublisherInfoList, void(
      uint32_t start,
      uint32_t limit,
      const ledger::ActivityInfoFilter& filter,
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD1(SetRewardsMainEnabled, void(bool enabled));

  MOCK_METHOD1(SetPublisherMinVisitTime, void(uint64_t duration_in_seconds));

  MOCK_METHOD1(SetPublisherMinVisits, void(unsigned int visits));

  MOCK_METHOD1(SetPublisherAllowNonVerified, void(bool allow));

  MOCK_METHOD1(SetPublisherAllowVideos, void(bool allow));

  MOCK_METHOD1(SetContributionAmount, void(double amount));

  MOCK_METHOD0(SetUserChangedContribution, void());

  MOCK_METHOD1(SetAutoContribute, void(bool enabled));

  MOCK_METHOD3(SetBalanceReport, void(
      ledger::ACTIVITY_MONTH month,
      int year,
      const ledger::BalanceReportInfo& report_info));

  MOCK_CONST_METHOD0(GetBATAddress, const std::string&());

  MOCK_CONST_METHOD0(GetBTCAddress, const std::string&());

  MOCK_CONST_METHOD0(GetETHAddress, const std::string&());

  MOCK_CONST_METHOD0(GetLTCAddress, const std::string&());

  MOCK_CONST_METHOD0(GetReconcileStamp, uint64_t());

  MOCK_CONST_METHOD0(GetRewardsMainEnabled, bool());

  MOCK_CONST_METHOD0(GetPublisherMinVisitTime, uint64_t());

  MOCK_CONST_METHOD0(GetPublisherMinVisits, unsigned int());

  MOCK_CONST_METHOD0(GetNumExcludedSites, unsigned int());

  MOCK_CONST_METHOD0(GetPublisherAllowNonVerified, bool());

  MOCK_CONST_METHOD0(GetPublisherAllowVideos, bool());

  MOCK_CONST_METHOD0(GetContributionAmount, double());

  MOCK_CONST_METHOD0(GetAutoContribute, bool());

  MOCK_CONST_METHOD0(FetchWalletProperties, void());

  MOCK_CONST_METHOD2(FetchGrant, void(const std::string& lang,
                                const std::string& paymentId));

  MOCK_CONST_METHOD1(SolveGrantCaptcha, void(const std::string& solution));

  MOCK_CONST_METHOD0(GetGrantCaptcha, void());

  MOCK_CONST_METHOD0(GetWalletPassphrase,  std::string());

  MOCK_CONST_METHOD3(GetBalanceReport, bool(
      ledger::ACTIVITY_MONTH month,
      int year,
      ledger::BalanceReportInfo* report_info));

  MOCK_CONST_METHOD0(GetAllBalanceReports,
      std::map<std::string, ledger::BalanceReportInfo>());

  MOCK_CONST_METHOD1(RecoverWallet, void(const std::string& passPhrase));

  MOCK_METHOD4(SaveMediaVisit, void(const std::string& publisher_id,
                                    const ledger::VisitData& visit_data,
                                    const uint64_t& duration,
                                    const uint64_t window_id));

  MOCK_METHOD2(SetPublisherExclude, void(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude));

  MOCK_METHOD3(SetPublisherPanelExclude, void(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude,
      uint64_t windowId));

  MOCK_METHOD0(RestorePublishers, void());

  MOCK_CONST_METHOD0(IsWalletCreated, bool());

  MOCK_METHOD2(GetPublisherActivityFromUrl, void(
      uint64_t windowId,
      const ledger::VisitData& visit_data));

  MOCK_METHOD4(SetBalanceReportItem, void(ledger::ACTIVITY_MONTH month,
                                          int year,
                                          ledger::ReportType type,
                                          const std::string& probi));

  MOCK_METHOD2(GetPublisherBanner, void(
      const std::string& publisher_id,
      ledger::PublisherBannerCallback callback));

  MOCK_METHOD0(GetBalance, double());

  MOCK_METHOD6(OnReconcileCompleteSuccess, void(
      const std::string& viewing_id,
      const ledger::REWARDS_CATEGORY category,
      const std::string& probi,
      const ledger::ACTIVITY_MONTH month,
      const int year,
      const uint32_t date));

  MOCK_METHOD1(RemoveRecurring, void(const std::string& publisher_key));

  MOCK_METHOD0(GetDefaultContributionAmount, double());

  MOCK_CONST_METHOD0(GetBootStamp, uint64_t());
};

std::unique_ptr<Profile> CreateBraveRewardsProfile(const base::FilePath& path);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_UTIL_H_

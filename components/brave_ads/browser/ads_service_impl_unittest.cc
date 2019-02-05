/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/scoped_temp_dir.h"
#include "extensions/browser/test_event_router.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_ads/browser/test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"

// npm run test -- brave_unit_tests --filter=AdsServiceTest.*

using namespace brave_ads;
using namespace brave_rewards;
using ::testing::_;

class MockRewardsService : public RewardsService {
 public:
  MockRewardsService() {}
  ~MockRewardsService() {}

  MOCK_METHOD0(CreateWallet, void());
  MOCK_METHOD0(FetchWalletProperties, void());
  MOCK_METHOD7(GetContentSiteList, void(uint32_t,
                                        uint32_t,
                                        uint64_t,
                                        uint64_t,
                                        bool,
                                        uint32_t,
                                        const GetContentSiteListCallback&));
  MOCK_METHOD2(FetchGrants, void(const std::string&, const std::string&));
  MOCK_METHOD2(GetGrantCaptcha, void(const std::string&, const std::string&));
  MOCK_CONST_METHOD2(SolveGrantCaptcha, void(const std::string&, const std::string&));
  MOCK_METHOD1(GetWalletPassphrase, void(const GetWalletPassphraseCallback&));
  MOCK_METHOD1(GetExcludedPublishersNumber, void(const GetExcludedPublishersNumberCallback&));
  MOCK_CONST_METHOD1(RecoverWallet, void(const std::string));
  MOCK_CONST_METHOD1(ExcludePublisher, void(const std::string));
  MOCK_METHOD0(RestorePublishers, void());
  MOCK_METHOD2(OnLoad, void(SessionID, const GURL&));
  MOCK_METHOD1(OnUnload, void(SessionID));
  MOCK_METHOD1(OnShow, void(SessionID));
  MOCK_METHOD1(OnHide, void(SessionID));
  MOCK_METHOD1(OnForeground, void(SessionID));
  MOCK_METHOD1(OnBackground, void(SessionID));
  MOCK_METHOD1(OnMediaStart, void(SessionID));
  MOCK_METHOD1(OnMediaStop, void(SessionID));
  MOCK_METHOD4(OnXHRLoad, void(SessionID,
                               const GURL&,
                               const GURL&,
                               const GURL&));
  MOCK_METHOD5(OnPostData, void(SessionID,
                               const GURL&,
                               const GURL&,
                               const GURL&,
                               const std::string&));
  MOCK_METHOD1(GetReconcileStamp, void(const GetReconcileStampCallback&));
  MOCK_METHOD1(GetAddresses, void(const GetAddressesCallback&));
  MOCK_METHOD1(SetRewardsMainEnabled, void(bool));
  MOCK_METHOD1(GetPublisherMinVisitTime, void(const GetPublisherMinVisitTimeCallback&));
  MOCK_CONST_METHOD1(SetPublisherMinVisitTime, void(uint64_t));
  MOCK_METHOD1(GetPublisherMinVisits, void(const GetPublisherMinVisitsCallback&));
  MOCK_CONST_METHOD1(SetPublisherMinVisits, void(unsigned int));
  MOCK_METHOD1(GetPublisherAllowNonVerified, void(const GetPublisherAllowNonVerifiedCallback&));
  MOCK_CONST_METHOD1(SetPublisherAllowNonVerified, void(bool));
  MOCK_METHOD1(GetPublisherAllowVideos, void(const GetPublisherAllowVideosCallback&));
  MOCK_CONST_METHOD1(SetPublisherAllowVideos, void(bool));
  MOCK_CONST_METHOD1(SetContributionAmount, void(double));
  MOCK_CONST_METHOD0(SetUserChangedContribution, void());
  MOCK_METHOD1(GetAutoContribute, void(GetAutoContributeCallback));
  MOCK_CONST_METHOD1(SetAutoContribute, void(bool));
  MOCK_METHOD2(SetTimer, void(uint64_t, uint32_t*));
  MOCK_METHOD1(GetAllBalanceReports, void(const GetAllBalanceReportsCallback&));
  MOCK_METHOD0(GetCurrentBalanceReport, void());
  MOCK_METHOD1(IsWalletCreated, void(const IsWalletCreatedCallback&));
  MOCK_METHOD4(GetPublisherActivityFromUrl, void(uint64_t,
                                                 const std::string&,
                                                 const std::string&,
                                                 const std::string&));
  MOCK_METHOD1(GetContributionAmount, void(const GetContributionAmountCallback&));
  MOCK_METHOD2(GetPublisherBanner, void(const std::string&, GetPublisherBannerCallback));
  MOCK_METHOD4(OnDonate, void(const std::string&,
                              int,
                              bool,
                              const ledger::PublisherInfo*));
  MOCK_METHOD4(OnDonate, void(const std::string&,
                              int,
                              bool,
                              std::unique_ptr<brave_rewards::ContentSite>));
  MOCK_METHOD1(RemoveRecurringTip, void(const std::string&));
  MOCK_METHOD1(GetRecurringTipsUI, void(GetRecurringTipsCallback));
  MOCK_METHOD1(GetOneTimeTipsUI, void(GetOneTimeTipsCallback));
  MOCK_METHOD2(SetContributionAutoInclude, void(const std::string&, bool));
  MOCK_CONST_METHOD0(GetNotificationService, RewardsNotificationService*());
  MOCK_METHOD0(CheckImported, bool());
  MOCK_METHOD0(SetBackupCompleted, void());
  MOCK_METHOD1(GetAutoContributeProps, void(const GetAutoContributePropsCallback&));
  MOCK_METHOD1(GetPendingContributionsTotal, void(const GetPendingContributionsTotalCallback&));
  MOCK_CONST_METHOD1(GetRewardsMainEnabled, void(const GetRewardsMainEnabledCallback&));
  MOCK_METHOD1(SetCatalogIssuers, void(const std::string&));
  MOCK_METHOD1(ConfirmAd, void(const std::string&));
  MOCK_METHOD1(GetRewardsInternalsInfo, void(GetRewardsInternalsInfoCallback));
  MOCK_METHOD1(GetAddressesForPaymentId, void(const GetAddressesCallback&));
  MOCK_METHOD1(GetConfirmationsHistory, void(ConfirmationsHistoryCallback));
  MOCK_METHOD2(SaveRecurringTip, void(const std::string&, const int));
  MOCK_METHOD2(RefreshPublisher, void(const std::string&, RefreshPublisherCallback));
};

class AdsServiceTest : public testing::Test {
 public:
  AdsServiceTest() {}
  ~AdsServiceTest() override {}

  AdsService* ads_service_;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    profile_ = CreateBraveAdsProfile(temp_dir_.GetPath());
    ASSERT_TRUE(profile_.get() != NULL);
    RewardsServiceFactory::SetServiceForTesting(new MockRewardsService());
    rewards_service_ = static_cast<MockRewardsService*>(
        RewardsServiceFactory::GetForProfile(profile()));
    ads_service_ = AdsServiceFactory::GetForProfile(profile());
    ASSERT_TRUE(AdsServiceFactory::GetInstance() != NULL);
    ASSERT_TRUE(ads_service() != NULL); 
  }

  void TearDown() override {
    profile_.reset();
    delete rewards_service_;
  }

  Profile* profile() { return profile_.get(); }
  AdsService* ads_service() { return ads_service_; }
  MockRewardsService* rewards_service() { return rewards_service_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<Profile> profile_;
  base::ScopedTempDir temp_dir_;
  MockRewardsService* rewards_service_;
};


TEST_F(AdsServiceTest, MaybeShowFirstLaunchNotification) {
  EXPECT_CALL(*rewards_service(), GetNotificationService())
      .Times(0);
  profile()->GetPrefs()->SetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled, false);
  ads_service_->MaybeShowFirstLaunchNotification();
}

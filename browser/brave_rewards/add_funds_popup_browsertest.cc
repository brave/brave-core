/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/add_funds_popup.h"

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_strip_types.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/referrer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

#include <array>
#include <map>
#include <string>

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;

namespace brave_rewards {
class RewardsNotificationService;
}

using brave_rewards::AddFundsPopup;
using brave_rewards::BalanceReport;
using brave_rewards::RewardsNotificationService;
using brave_rewards::RewardsService;
using content::MessageLoopRunner;
using content::OpenURLParams;
using content::Referrer;
using content::RenderFrameHost;
using content::RenderFrameHostImpl;
using content::WaitForLoadStop;
using content::WaitForLoadStopWithoutSuccessCheck;
using content::WebContents;
using content::WebContentsImpl;
using content::WebContentsObserver;

// RewardsService callbacks
using brave_rewards::GetAddressesCallback;
using brave_rewards::GetAllBalanceReportsCallback;
using brave_rewards::GetAutoContributeCallback;
using brave_rewards::GetAutoContributePropsCallback;
using brave_rewards::GetContentSiteListCallback;
using brave_rewards::GetContributionAmountCallback;
using brave_rewards::GetNumExcludedSitesCallback;
using brave_rewards::GetPendingContributionsTotalCallback;
using brave_rewards::GetPublisherAllowNonVerifiedCallback;
using brave_rewards::GetPublisherAllowVideosCallback;
using brave_rewards::GetPublisherMinVisitsCallback;
using brave_rewards::GetPublisherMinVisitTimeCallback;
using brave_rewards::GetReconcileStampCallback;
using brave_rewards::GetRewardsMainEnabledCallback;
using brave_rewards::GetWalletPassphraseCallback;
using brave_rewards::IsWalletCreatedCallback;

namespace {

constexpr char kUpholdWidget[] = "https://uphold-widget.brave.com";
constexpr char kUphold[] = "https://uphold.com";
constexpr char kNetverify[] = "https://netverify.com";
constexpr char kTypekit[] = "https://use.typekit.net";
constexpr char kFirstParty[] = "https://firstParty";
constexpr char kExample[] = "https://example.com";

const std::array<const std::string, 3> kHosts = {kUphold, kNetverify, kTypekit};

const std::map<std::string, std::string> kAddresses = {{"BTC", "0xA"},
                                                       {"BAT", "0xB"},
                                                       {"ETH", "0xC"},
                                                       {"LTC", "0xD"}};

// Mock RewardsService methods called by AddFundsPopup.
class MockRewardsService : public RewardsService {
 public:
  MOCK_METHOD0(CreateWallet, void());
  MOCK_METHOD0(FetchWalletProperties, void());
  MOCK_METHOD7(GetContentSiteList,
               void(uint32_t start,
                    uint32_t limit,
                    uint64_t min_visit_time,
                    uint64_t reconcile_stamp,
                    bool allow_non_verified,
                    uint32_t min_visits,
                    const GetContentSiteListCallback& callback));
  MOCK_METHOD2(FetchGrant,
               void(const std::string& lang, const std::string& paymentId));
  MOCK_METHOD0(GetGrantCaptcha, void());
  MOCK_CONST_METHOD1(SolveGrantCaptcha, void(const std::string& solution));
  MOCK_METHOD1(GetWalletPassphrase,
               void(const GetWalletPassphraseCallback& callback));
  MOCK_METHOD1(GetNumExcludedSites,
               void(const GetNumExcludedSitesCallback& callback));
  MOCK_CONST_METHOD1(RecoverWallet, void(const std::string passPhrase));
  MOCK_CONST_METHOD1(ExcludePublisher, void(const std::string publisherKey));
  MOCK_METHOD0(RestorePublishers, void());
  MOCK_METHOD2(OnLoad, void(SessionID tab_id, const GURL& gurl));
  MOCK_METHOD1(OnUnload, void(SessionID tab_id));
  MOCK_METHOD1(OnShow, void(SessionID tab_id));
  MOCK_METHOD1(OnHide, void(SessionID tab_id));
  MOCK_METHOD1(OnForeground, void(SessionID tab_id));
  MOCK_METHOD1(OnBackground, void(SessionID tab_id));
  MOCK_METHOD1(OnMediaStart, void(SessionID tab_id));
  MOCK_METHOD1(OnMediaStop, void(SessionID tab_id));
  MOCK_METHOD4(OnXHRLoad,
               void(SessionID tab_id,
                    const GURL& url,
                    const GURL& first_party_url,
                    const GURL& referrer));
  MOCK_METHOD5(OnPostData,
               void(SessionID tab_id,
                    const GURL& url,
                    const GURL& first_party_url,
                    const GURL& referrer,
                    const std::string& post_data));
  MOCK_METHOD1(GetReconcileStamp,
               void(const GetReconcileStampCallback& callback));
  MOCK_METHOD1(GetAddresses, void(const GetAddressesCallback& callback));
  MOCK_METHOD1(SetRewardsMainEnabled, void(bool enabled));
  MOCK_METHOD1(GetPublisherMinVisitTime,
               void(const GetPublisherMinVisitTimeCallback& callback));
  MOCK_CONST_METHOD1(SetPublisherMinVisitTime,
                     void(uint64_t duration_in_seconds));
  MOCK_METHOD1(GetPublisherMinVisits,
               void(const GetPublisherMinVisitsCallback& callback));
  MOCK_CONST_METHOD1(SetPublisherMinVisits, void(unsigned int visits));
  MOCK_METHOD1(GetPublisherAllowNonVerified,
               void(const GetPublisherAllowNonVerifiedCallback& callback));
  MOCK_CONST_METHOD1(SetPublisherAllowNonVerified, void(bool allow));
  MOCK_METHOD1(GetPublisherAllowVideos,
               void(const GetPublisherAllowVideosCallback& callback));
  MOCK_CONST_METHOD1(SetPublisherAllowVideos, void(bool allow));
  MOCK_CONST_METHOD1(SetContributionAmount, void(double amount));
  MOCK_CONST_METHOD0(SetUserChangedContribution, void());
  MOCK_METHOD1(GetAutoContribute,
               void(const GetAutoContributeCallback& callback));
  MOCK_CONST_METHOD1(SetAutoContribute, void(bool enabled));
  MOCK_METHOD2(SetTimer, void(uint64_t time_offset, uint32_t& timer_id));
  MOCK_METHOD1(GetAllBalanceReports,
               void(const GetAllBalanceReportsCallback& callback));
  MOCK_METHOD0(GetCurrentBalanceReport, void());
  MOCK_METHOD1(IsWalletCreated, void(const IsWalletCreatedCallback& callback));
  MOCK_METHOD4(GetPublisherActivityFromUrl,
               void(uint64_t windowId,
                    const std::string& url,
                    const std::string& favicon_url,
                    const std::string& publisher_blob));
  MOCK_METHOD1(GetContributionAmount,
               void(const GetContributionAmountCallback& callback));
  MOCK_METHOD1(GetPublisherBanner, void(const std::string& publisher_id));
  MOCK_METHOD4(OnDonate,
               void(const std::string& publisher_key,
                    int amount,
                    bool recurring,
                    const ledger::PublisherInfo* publisher_info));
  MOCK_METHOD4(OnDonate,
               void(const std::string& publisher_key,
                    int amount,
                    bool recurring,
                    std::unique_ptr<brave_rewards::ContentSite> site));
  MOCK_METHOD1(RemoveRecurring, void(const std::string& publisher_key));
  MOCK_METHOD0(UpdateRecurringDonationsList, void());
  MOCK_METHOD0(UpdateTipsList, void());
  MOCK_METHOD3(SetContributionAutoInclude,
               void(const std::string& publisher_key,
                    bool excluded,
                    uint64_t windowId));
  MOCK_CONST_METHOD0(GetNotificationService, RewardsNotificationService*());
  MOCK_METHOD0(CheckImported, bool());
  MOCK_METHOD0(SetBackupCompleted, void());
  MOCK_METHOD1(GetAutoContributeProps,
               void(const GetAutoContributePropsCallback& callback));
  MOCK_METHOD1(GetPendingContributionsTotal,
               void(const GetPendingContributionsTotalCallback& callback));
  MOCK_CONST_METHOD1(GetRewardsMainEnabled,
                     void(const GetRewardsMainEnabledCallback& callback));
  MOCK_METHOD1(GetAddressesForPaymentId,
               void(const GetAddressesCallback& callback));
};

class BraveAddFundsPopupTest : public InProcessBrowserTest {
 public:
  BraveAddFundsPopupTest() = default;
  ~BraveAddFundsPopupTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    Profile* profile = browser()->profile();
    DCHECK(profile);
    map_ = HostContentSettingsMapFactory::GetForProfile(profile);
    DCHECK(map_);
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void DisallowSetting(ContentSettingsType type,
                       const content_settings::ResourceIdentifier& id) {
    map_->SetContentSettingCustomScope(ContentSettingsPattern::Wildcard(),
                                       ContentSettingsPattern::Wildcard(), type,
                                       id, CONTENT_SETTING_BLOCK);
  }

  void DisallowDefaultSetting(ContentSettingsType type) {
    map_->SetDefaultContentSetting(type, CONTENT_SETTING_BLOCK);
  }

  void DisallowAll() {
    DisallowSetting(CONTENT_SETTINGS_TYPE_PLUGINS,
                    brave_shields::kFingerprinting);
    DisallowSetting(CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kReferrers);
    DisallowSetting(CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
    DisallowSetting(CONTENT_SETTINGS_TYPE_JAVASCRIPT, std::string());
    DisallowDefaultSetting(CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
    DisallowDefaultSetting(CONTENT_SETTINGS_TYPE_AUTOPLAY);
  }

  void CheckSetting(
      ContentSetting setting,
      const std::string& host,
      ContentSettingsType type,
      const content_settings::ResourceIdentifier& id = std::string(),
      const std::string& secondary = std::string()) const {
    EXPECT_EQ(setting,
              map_->GetContentSetting(GURL(host), GURL(secondary), type, id));
  }

  void CheckAllowed(
      const std::string& host,
      ContentSettingsType type,
      const content_settings::ResourceIdentifier& id = std::string(),
      const std::string& secondary = std::string()) const {
    CheckSetting(CONTENT_SETTING_ALLOW, host, type, id, secondary);
  }

  void CheckDisallowed(
      const std::string& host,
      ContentSettingsType type,
      const content_settings::ResourceIdentifier& id = std::string(),
      const std::string& secondary = std::string()) const {
    CheckSetting(CONTENT_SETTING_BLOCK, host, type, id, secondary);
  }

  void CheckDisallowed(const std::string& host) const {
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_PLUGINS,
                    brave_shields::kFingerprinting);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_PLUGINS,
                    brave_shields::kFingerprinting, kFirstParty);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_PLUGINS,
                    brave_shields::kReferrers);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_PLUGINS,
                    brave_shields::kCookies);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_PLUGINS,
                    brave_shields::kCookies, kFirstParty);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_JAVASCRIPT);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
    CheckDisallowed(host, CONTENT_SETTINGS_TYPE_AUTOPLAY);
  }

  void CheckDisallowed() const {
    CheckDisallowed(kUpholdWidget);
    for (const auto& host : kHosts)
      CheckDisallowed(host, CONTENT_SETTINGS_TYPE_JAVASCRIPT);
  }

  void CheckAllowed() const {
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_PLUGINS,
                 brave_shields::kFingerprinting);
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_PLUGINS,
                 brave_shields::kFingerprinting, kFirstParty);
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_PLUGINS,
                 brave_shields::kReferrers);
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_PLUGINS,
                 brave_shields::kCookies);
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_PLUGINS,
                 brave_shields::kCookies, kFirstParty);
    for (const auto& host : kHosts)
      CheckAllowed(host, CONTENT_SETTINGS_TYPE_JAVASCRIPT);
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
    CheckAllowed(kUpholdWidget, CONTENT_SETTINGS_TYPE_AUTOPLAY);
  }

  MockRewardsService& rewards_service() { return mock_rewards_service_; }

 private:
  NiceMock<MockRewardsService> mock_rewards_service_;
  HostContentSettingsMap* map_;

  DISALLOW_COPY_AND_ASSIGN(BraveAddFundsPopupTest);
};

class BrowserListRemovalObserver : public BrowserListObserver {
 public:
  explicit BrowserListRemovalObserver(Browser* browser)
      : message_loop_runner_(new MessageLoopRunner),
        removed_(false),
        browser_(browser) {
    BrowserList::AddObserver(this);
  }

  ~BrowserListRemovalObserver() override { BrowserList::RemoveObserver(this); }

  bool removed() const { return removed_; }

  void Wait() {
    if (removed_)
      return;
    message_loop_runner_->Run();
  }

  // BrowserListObserver implementation:
  void OnBrowserRemoved(Browser* browser) override {
    if (browser_ == browser) {
      CHECK(!removed_);
      removed_ = true;
    }

    if (removed_ && message_loop_runner_->loop_running()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, message_loop_runner_->QuitClosure());
    }
  }

 private:
  scoped_refptr<MessageLoopRunner> message_loop_runner_;
  bool removed_;
  Browser* browser_;
};

ACTION_TEMPLATE(InvokeCallbackArgument,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p0)) {
  std::get<k>(args).Run(p0);
}

}  // namespace

// Tests that content permissions are altered to allow fingerprinting, cookies,
// scripts, access to camera, and autoplay. Tests that these permissions are
// returned to original values when the popup closes.
IN_PROC_BROWSER_TEST_F(BraveAddFundsPopupTest, TestAddFundsPopupClosed) {
  // Set all permissions to disallow.
  DisallowAll();
  CheckDisallowed();

  // Initially loaded tab is on about:blank which would cause a popup to be
  // blocked. Navigate to brave://rewards to avoid popup blocker.
  OpenURLParams paramsInitiator(GURL("brave://rewards"), Referrer(),
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_TYPED, false);
  WebContents* initiator = browser()->OpenURL(paramsInitiator);
  DCHECK(initiator);
  EXPECT_TRUE(WaitForLoadStop(initiator));

  // Show Add Funds popup.
  auto popup = std::make_unique<AddFundsPopup>();
  EXPECT_CALL(rewards_service(), GetAddresses(_))
      .Times(1)
      .WillOnce(InvokeCallbackArgument<0>(kAddresses));
  popup->ShowPopup(initiator, &rewards_service());
  WaitForLoadStopWithoutSuccessCheck(popup->add_funds_popup_);

  // Check that all permissions are allowed.
  CheckAllowed();

  // Check that permissions are blocked for a random site.
  CheckDisallowed(kExample);

  // Close the popup.
  Browser* popup_browser =
      chrome::FindBrowserWithWebContents(popup->add_funds_popup_);
  DCHECK(popup_browser);

  BrowserListRemovalObserver blro(popup_browser);

  int index = popup_browser->tab_strip_model()->GetIndexOfWebContents(
      popup->add_funds_popup_);
  DCHECK(index != -1);
  TabStrip* tab_strip =
      BrowserView::GetBrowserViewForBrowser(popup_browser)->tabstrip();
  DCHECK(tab_strip);

  EXPECT_CALL(rewards_service(), FetchWalletProperties()).Times(1);

  tab_strip->CloseTab(tab_strip->tab_at(index), CLOSE_TAB_FROM_MOUSE);
  blro.Wait();
  EXPECT_TRUE(blro.removed());

  // Check that the popup has closed.
  EXPECT_EQ(nullptr, popup->add_funds_popup_);

  // Check that all permissions are disallowed.
  CheckDisallowed();
}

// Test that if the popup initiator tab is closed, the popup is closed as well.
// The initiator tab holds a unique_ptr to the popup, so this test just checks
// that the popup is closed when the pointer is reset.
IN_PROC_BROWSER_TEST_F(BraveAddFundsPopupTest, TestAddFundsPopupDeleted) {
  // Set all permissions to disallow.
  DisallowAll();
  CheckDisallowed();

  // Initially loaded tab is on about:blank which would cause a popup to be
  // blocked. Navigate to brave://rewards to avoid popup blocker.
  OpenURLParams paramsInitiator(GURL("brave://rewards"), Referrer(),
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_TYPED, false);
  WebContents* initiator = browser()->OpenURL(paramsInitiator);
  DCHECK(initiator);
  EXPECT_TRUE(WaitForLoadStop(initiator));

  // Show Add Funds popup.
  auto popup = std::make_unique<AddFundsPopup>();
  EXPECT_CALL(rewards_service(), GetAddresses(_))
      .Times(1)
      .WillOnce(InvokeCallbackArgument<0>(kAddresses));
  popup->ShowPopup(initiator, &rewards_service());
  WaitForLoadStopWithoutSuccessCheck(popup->add_funds_popup_);

  // Check that all permissions are allowed.
  CheckAllowed();

  // Simulate initiator tab's web UI going away by resetting the popup pointer.
  Browser* popup_browser =
      chrome::FindBrowserWithWebContents(popup->add_funds_popup_);
  DCHECK(popup_browser);
  BrowserListRemovalObserver blro(popup_browser);
  popup.reset(nullptr);
  blro.Wait();
  EXPECT_TRUE(blro.removed());

  // Check that all permissions are disallowed.
  CheckDisallowed();
}

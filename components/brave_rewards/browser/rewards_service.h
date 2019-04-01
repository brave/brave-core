/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/components/brave_rewards/browser/auto_contribution_props.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/rewards_internals_info.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace ads {
struct IssuersInfo;
struct NotificationInfo;
}

namespace ledger {
struct PublisherInfo;
}

namespace content {
class NavigationHandle;
}

namespace brave_rewards {

bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer);

class RewardsNotificationService;
class RewardsServiceObserver;

using GetContentSiteListCallback =
    base::Callback<void(std::unique_ptr<ContentSiteList>,
        uint32_t /* next_record */)>;
using GetAllBalanceReportsCallback = base::Callback<void(
    const std::map<std::string, brave_rewards::BalanceReport>&)>;
using GetWalletPassphraseCallback = base::Callback<void(const std::string&)>;
using GetContributionAmountCallback = base::Callback<void(double)>;
using GetAddressesCallback = base::Callback<void(
    const std::map<std::string, std::string>&)>;
using GetExcludedPublishersNumberCallback = base::Callback<void(uint32_t)>;
using GetAutoContributePropsCallback = base::Callback<void(
    std::unique_ptr<brave_rewards::AutoContributeProps>)>;
using GetPublisherMinVisitTimeCallback = base::Callback<void(uint64_t)>;
using GetPublisherMinVisitsCallback = base::Callback<void(uint32_t)>;
using GetPublisherAllowNonVerifiedCallback = base::Callback<void(bool)>;
using GetPublisherAllowVideosCallback = base::Callback<void(bool)>;
using GetAutoContributeCallback = base::OnceCallback<void(bool)>;
using GetReconcileStampCallback = base::Callback<void(uint64_t)>;
using IsWalletCreatedCallback = base::Callback<void(bool)>;
using GetPendingContributionsTotalCallback = base::Callback<void(double)>;
using GetRewardsMainEnabledCallback = base::Callback<void(bool)>;
using ConfirmationsHistoryCallback = base::Callback<void(int, double)>;
using GetRewardsInternalsInfoCallback = base::OnceCallback<void(
    std::unique_ptr<brave_rewards::RewardsInternalsInfo>)>;
using GetRecurringTipsCallback = base::OnceCallback<void(
    std::unique_ptr<brave_rewards::ContentSiteList>)>;

class RewardsService : public KeyedService {
 public:
  RewardsService();
  ~RewardsService() override;

  virtual void CreateWallet() = 0;
  virtual void FetchWalletProperties() = 0;
  virtual void GetContentSiteList(
      uint32_t start,
      uint32_t limit,
      uint64_t min_visit_time,
      uint64_t reconcile_stamp,
      bool allow_non_verified,
      uint32_t min_visits,
      const GetContentSiteListCallback& callback) = 0;
  virtual void FetchGrants(const std::string& lang,
                           const std::string& paymentId) = 0;
  virtual void GetGrantCaptcha(
      const std::string& promotion_id,
      const std::string& promotion_type) = 0;
  virtual void SolveGrantCaptcha(const std::string& solution,
                                 const std::string& promotionId) const = 0;
  virtual void GetWalletPassphrase(
      const GetWalletPassphraseCallback& callback) = 0;
  virtual void GetExcludedPublishersNumber(
      const GetExcludedPublishersNumberCallback& callback) = 0;
  virtual void RecoverWallet(const std::string passPhrase) const = 0;
  virtual void ExcludePublisher(const std::string publisherKey) const = 0;
  virtual void RestorePublishers() = 0;
  virtual void OnLoad(SessionID tab_id, const GURL& gurl) = 0;
  virtual void OnUnload(SessionID tab_id) = 0;
  virtual void OnShow(SessionID tab_id) = 0;
  virtual void OnHide(SessionID tab_id) = 0;
  virtual void OnForeground(SessionID tab_id) = 0;
  virtual void OnBackground(SessionID tab_id) = 0;
  virtual void OnMediaStart(SessionID tab_id) = 0;
  virtual void OnMediaStop(SessionID tab_id) = 0;
  virtual void OnXHRLoad(SessionID tab_id,
                         const GURL& url,
                         const GURL& first_party_url,
                         const GURL& referrer) = 0;
  virtual void OnPostData(SessionID tab_id,
                          const GURL& url,
                          const GURL& first_party_url,
                          const GURL& referrer,
                          const std::string& post_data) = 0;

  virtual void GetReconcileStamp(
      const GetReconcileStampCallback& callback) = 0;
  virtual void GetAddresses(const GetAddressesCallback& callback) = 0;
  virtual void SetRewardsMainEnabled(bool enabled) = 0;
  virtual void GetPublisherMinVisitTime(
      const GetPublisherMinVisitTimeCallback& callback) = 0;
  virtual void SetPublisherMinVisitTime(uint64_t duration_in_seconds) const = 0;
  virtual void GetPublisherMinVisits(
      const GetPublisherMinVisitsCallback& callback) = 0;
  virtual void SetPublisherMinVisits(unsigned int visits) const = 0;
  virtual void GetPublisherAllowNonVerified(
      const GetPublisherAllowNonVerifiedCallback& callback) = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) const = 0;
  virtual void GetPublisherAllowVideos(
      const GetPublisherAllowVideosCallback& callback) = 0;
  virtual void SetPublisherAllowVideos(bool allow) const = 0;
  virtual void SetContributionAmount(double amount) const = 0;
  virtual void SetUserChangedContribution() const = 0;
  virtual void GetAutoContribute(
      GetAutoContributeCallback callback) = 0;
  virtual void SetAutoContribute(bool enabled) const = 0;
  virtual void SetTimer(uint64_t time_offset, uint32_t* timer_id) = 0;
  virtual void GetAllBalanceReports(
      const GetAllBalanceReportsCallback& callback) = 0;
  virtual void GetCurrentBalanceReport() = 0;
  virtual void IsWalletCreated(const IsWalletCreatedCallback& callback) = 0;
  virtual void GetPublisherActivityFromUrl(
      uint64_t windowId,
      const std::string& url,
      const std::string& favicon_url,
      const std::string& publisher_blob) = 0;
  virtual void GetContributionAmount(
      const GetContributionAmountCallback& callback) = 0;
  virtual void GetPublisherBanner(const std::string& publisher_id) = 0;
  virtual void OnDonate(const std::string& publisher_key, int amount,
      bool recurring, const ledger::PublisherInfo* publisher_info = NULL) = 0;
  virtual void OnDonate(const std::string& publisher_key, int amount,
      bool recurring, std::unique_ptr<brave_rewards::ContentSite> site) = 0;
  virtual void RemoveRecurringTip(const std::string& publisher_key) = 0;
  virtual void GetRecurringTipsUI(GetRecurringTipsCallback callback) = 0;
  virtual void GetOneTimeTips() = 0;
  virtual void SetContributionAutoInclude(
    const std::string& publisher_key, bool excluded) = 0;
  virtual RewardsNotificationService* GetNotificationService() const = 0;
  virtual bool CheckImported() = 0;
  virtual void SetBackupCompleted() = 0;
  virtual void GetAutoContributeProps(
    const GetAutoContributePropsCallback& callback) = 0;
  virtual void GetPendingContributionsTotal(
    const GetPendingContributionsTotalCallback& callback) = 0;
  virtual void GetRewardsMainEnabled(
    const GetRewardsMainEnabledCallback& callback) const = 0;
  // TODO(Terry Mancey): remove this hack when ads is moved to the same process
  // as ledger
  virtual void SetCatalogIssuers(const std::string& json) = 0;
  virtual void ConfirmAd(const std::string& json) = 0;
  virtual void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) = 0;

  virtual void GetAddressesForPaymentId(
      const GetAddressesCallback& callback) = 0;
  virtual void GetConfirmationsHistory(
      brave_rewards::ConfirmationsHistoryCallback callback) = 0;

  void AddObserver(RewardsServiceObserver* observer);
  void RemoveObserver(RewardsServiceObserver* observer);

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 protected:
  base::ObserverList<RewardsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsService);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_

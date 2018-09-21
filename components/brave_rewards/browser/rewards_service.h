/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_SERVICE_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_SERVICE_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}

namespace brave_rewards {

bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer);

class RewardsServiceObserver;

using GetContentSiteListCallback =
    base::Callback<void(std::unique_ptr<ContentSiteList>,
        uint32_t /* next_record */)>;

class RewardsService : public KeyedService {
 public:
  RewardsService();
  ~RewardsService() override;

  virtual void CreateWallet() = 0;
  virtual void GetWalletProperties() = 0;
  virtual void GetContentSiteList(uint32_t start,
                                  uint32_t limit,
                                const GetContentSiteListCallback& callback) = 0;
  virtual void GetGrant(const std::string& lang, const std::string& paymentId) = 0;
  virtual void GetGrantCaptcha() = 0;
  virtual void SolveGrantCaptcha(const std::string& solution) const = 0;
  virtual std::string GetWalletPassphrase() const = 0;
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

  virtual uint64_t GetReconcileStamp() const = 0;
  virtual std::map<std::string, std::string> GetAddresses() const = 0;
  virtual void SetRewardsMainEnabled(bool enabled) const = 0;
  virtual void SetPublisherMinVisitTime(uint64_t duration_in_seconds) const = 0;
  virtual void SetPublisherMinVisits(unsigned int visits) const = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) const = 0;
  virtual void SetPublisherAllowVideos(bool allow) const = 0;
  virtual void SetContributionAmount(double amount) const = 0;
  virtual void SetUserChangedContribution() const = 0;
  virtual void SetAutoContribute(bool enabled) const = 0;
  virtual void SetTimer(uint64_t time_offset, uint32_t& timer_id) = 0;
  virtual std::map<std::string, brave_rewards::BalanceReport> GetAllBalanceReports() = 0;
  virtual bool IsWalletCreated() = 0;
  virtual void GetPublisherActivityFromUrl(uint64_t windowId, const std::string& url) = 0;

  void AddObserver(RewardsServiceObserver* observer);
  void RemoveObserver(RewardsServiceObserver* observer);

 protected:
  base::ObserverList<RewardsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsService);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_SERVICE_

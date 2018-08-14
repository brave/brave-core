/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_SERVICE_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_SERVICE_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}

namespace brave_rewards {

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
  virtual void GetPromotion(const std::string& lang, const std::string& paymentId) = 0;
  virtual void GetPromotionCaptcha() = 0;
  virtual void SolvePromotionCaptcha(const std::string& solution) const = 0;
  virtual std::string GetWalletPassphrase() const = 0;
  virtual void RecoverWallet(const std::string passPhrase) const = 0;
  virtual void OnLoad(SessionID tab_id, const GURL& gurl) = 0;
  virtual void OnUnload(SessionID tab_id) = 0;
  virtual void OnShow(SessionID tab_id) = 0;
  virtual void OnHide(SessionID tab_id) = 0;
  virtual void OnForeground(SessionID tab_id) = 0;
  virtual void OnBackground(SessionID tab_id) = 0;
  virtual void OnMediaStart(SessionID tab_id) = 0;
  virtual void OnMediaStop(SessionID tab_id) = 0;
  virtual void OnXHRLoad(SessionID tab_id, const GURL& url) = 0;

  void AddObserver(RewardsServiceObserver* observer);
  void RemoveObserver(RewardsServiceObserver* observer);

 protected:
  base::ObserverList<RewardsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsService);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_SERVICE_

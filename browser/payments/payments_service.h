/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/browser/payments/content_site.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}

namespace payments {

class PaymentsServiceObserver;

using GetContentSiteListCallback =
    base::Callback<void(std::unique_ptr<ContentSiteList>,
        uint32_t /* next_record */)>;

class PaymentsService : public KeyedService {
 public:
  PaymentsService();
  ~PaymentsService() override;

  virtual void CreateWallet() = 0;
  virtual void GetWalletProperties() = 0;
  virtual void GetContentSiteList(uint32_t start,
                                  uint32_t limit,
                                const GetContentSiteListCallback& callback) = 0;
  virtual void GetPromotion(const std::string& lang, const std::string& paymentId) = 0;
  virtual void GetPromotionCaptcha() = 0;
  virtual void OnLoad(SessionID tab_id, const GURL& gurl) = 0;
  virtual void OnUnload(SessionID tab_id) = 0;
  virtual void OnShow(SessionID tab_id) = 0;
  virtual void OnHide(SessionID tab_id) = 0;
  virtual void OnForeground(SessionID tab_id) = 0;
  virtual void OnBackground(SessionID tab_id) = 0;
  virtual void OnMediaStart(SessionID tab_id) = 0;
  virtual void OnMediaStop(SessionID tab_id) = 0;
  virtual void OnXHRLoad(SessionID tab_id, const GURL& url) = 0;

  void AddObserver(PaymentsServiceObserver* observer);
  void RemoveObserver(PaymentsServiceObserver* observer);

 protected:
  base::ObserverList<PaymentsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentsService);
};

}  // namespace payments

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_pages.h"

#include "brave/browser/payments/payments_service.h"
#include "brave/browser/payments/payments_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "url/gurl.h"

namespace brave {

void ShowBraveRewards(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIRewardsURL)));
}

void ShowBraveAdblock(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIAdblockURL)));
}

void ShowBravePayments(Browser* browser) {
  payments::PaymentsService* payments_service =
      PaymentsServiceFactory::GetForProfile(browser->profile());

  if (payments_service) {
    payments_service->CreateWallet();
    // wallet is not created at this point. Ledger library needs to be updated
    // to provide some kind of callback when it is actually finished
  }
}

}  // namespace brave

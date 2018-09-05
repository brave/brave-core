/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_profile_network_delegate.h"

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_httpse_network_delegate_helper.h"
#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"
#endif

BraveProfileNetworkDelegate::BraveProfileNetworkDelegate(
    extensions::EventRouterForwarder* event_router) :
    BraveNetworkDelegateBase(event_router) {
  brave::OnBeforeURLRequestCallback callback =
      base::Bind(
          brave::OnBeforeURLRequest_SiteHacksWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::Bind(brave::OnBeforeURLRequest_HttpsePreFileWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::Bind(brave::OnBeforeURLRequest_CommonStaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  callback = base::Bind(brave_rewards::OnBeforeURLRequest);
  before_url_request_callbacks_.push_back(callback);
#endif

  brave::OnBeforeStartTransactionCallback start_transactions_callback =
      base::Bind(brave::OnBeforeStartTransaction_SiteHacksWork);
  before_start_transaction_callbacks_.push_back(start_transactions_callback);
}

BraveProfileNetworkDelegate::~BraveProfileNetworkDelegate() {
}

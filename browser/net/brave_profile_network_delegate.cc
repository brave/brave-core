/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_profile_network_delegate.h"

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/cookie_network_delegate_helper.h"
#include "brave/browser/net/brave_httpse_network_delegate_helper.h"
#include "brave/browser/net/brave_referrals_network_delegate_helper.h"
#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"
#include "brave/browser/net/brave_tor_network_delegate_helper.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"
#endif
#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"

BraveProfileNetworkDelegate::BraveProfileNetworkDelegate(
    extensions::EventRouterForwarder* event_router) :
    BraveNetworkDelegateBase(event_router) {
  brave::OnBeforeURLRequestCallback
  callback =
      base::Bind(brave::OnBeforeURLRequest_SiteHacksWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::Bind(brave::OnBeforeURLRequest_AdBlockTPPreWork);
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

  callback = base::Bind(brave::OnBeforeURLRequest_TorWork);
  before_url_request_callbacks_.push_back(callback);

  brave::OnBeforeStartTransactionCallback start_transaction_callback =
      base::Bind(brave::OnBeforeStartTransaction_SiteHacksWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

  start_transaction_callback =
      base::Bind(brave::OnBeforeStartTransaction_ReferralsWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

  brave::OnHeadersReceivedCallback headers_received_callback =
      base::Bind(
          webtorrent::OnHeadersReceived_TorrentRedirectWork);
  headers_received_callbacks_.push_back(headers_received_callback);

  brave::OnCanGetCookiesCallback get_cookies_callback =
      base::Bind(brave::OnCanGetCookiesForBraveShields);
  can_get_cookies_callbacks_.push_back(get_cookies_callback);

  brave::OnCanSetCookiesCallback set_cookies_callback =
      base::Bind(brave::OnCanSetCookiesForBraveShields);
  can_set_cookies_callbacks_.push_back(set_cookies_callback);
}

BraveProfileNetworkDelegate::~BraveProfileNetworkDelegate() {
}

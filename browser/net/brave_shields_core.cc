/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_shields_core.h"

#include <algorithm>
#include <utility>

#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/brave_stp_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"


#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/cookie_network_delegate_helper.h"
#include "brave/browser/net/brave_httpse_network_delegate_helper.h"
#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/browser/net/brave_referrals_network_delegate_helper.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE)
#include "brave/browser/net/brave_translate_redirect_network_delegate_helper.h"
#endif

BraveShieldsCore* BraveShieldsCore::GetInstance() {
  static BraveShieldsCore* instance = new BraveShieldsCore;
  return instance;
}


BraveShieldsCore::BraveShieldsCore() {
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

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE)
  callback = base::BindRepeating(
      brave::OnBeforeURLRequest_TranslateRedirectWork);
  before_url_request_callbacks_.push_back(callback);
#endif

  brave::OnBeforeStartTransactionCallback start_transaction_callback =
      base::Bind(brave::OnBeforeStartTransaction_SiteHacksWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  start_transaction_callback =
      base::Bind(brave::OnBeforeStartTransaction_ReferralsWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  brave::OnHeadersReceivedCallback headers_received_callback =
      base::Bind(
          webtorrent::OnHeadersReceived_TorrentRedirectWork);
  headers_received_callbacks_.push_back(headers_received_callback);
#endif

  brave::OnCanGetCookiesCallback get_cookies_callback =
      base::Bind(brave::OnCanGetCookiesForBraveShields);
  can_get_cookies_callbacks_.push_back(get_cookies_callback);

  brave::OnCanSetCookiesCallback set_cookies_callback =
      base::Bind(brave::OnCanSetCookiesForBraveShields);
  can_set_cookies_callbacks_.push_back(set_cookies_callback);
}

BraveShieldsCore::~BraveShieldsCore() {

}

bool BraveShieldsCore::IsRequestIdentifierValid(uint64_t request_identifier) {
  // TODO(iefremov)
  return true;
}

int BraveShieldsCore::OnBeforeURLRequest(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  if (before_url_request_callbacks_.empty()) {
    return net::OK;
  }
  ctx->new_url = new_url;
  ctx->event_type = brave::kOnBeforeRequest;
  callbacks_[ctx->request_identifier] = std::move(callback);
  RunNextCallback(ctx);
  return net::ERR_IO_PENDING;
}

int BraveShieldsCore::OnBeforeStartTransaction(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  if (before_start_transaction_callbacks_.empty()) {
    return net::OK;
  }
  ctx->event_type = brave::kOnBeforeStartTransaction;
  ctx->headers = headers;
  ctx->referral_headers_list = referral_headers_list_.get();
  callbacks_[ctx->request_identifier] = std::move(callback);
  RunNextCallback(ctx);
  return net::ERR_IO_PENDING;
}

int BraveShieldsCore::OnHeadersReceived(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  if (!ctx->tab_origin.is_empty()) {
    brave::RemoveTrackableSecurityHeadersForThirdParty(
        ctx->request_url, url::Origin::Create(ctx->tab_origin),
        original_response_headers, override_response_headers);
  }

  if (headers_received_callbacks_.empty()) {
    return net::OK;
  }

  callbacks_[ctx->request_identifier] = std::move(callback);
  ctx->event_type = brave::kOnHeadersReceived;
  ctx->original_response_headers = original_response_headers;
  ctx->override_response_headers = override_response_headers;
  ctx->allowed_unsafe_redirect_url = allowed_unsafe_redirect_url;

  // Return ERR_IO_PENDING and run callbacks later by posting a task.
  // URLRequestHttpJob::awaiting_callback_ will be set to true after we
  // return net::ERR_IO_PENDING here, callbacks need to be run later than this
  // to set awaiting_callback_ back to false.
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::Bind(&BraveShieldsCore::RunNextCallback,
                 base::Unretained(this), ctx));
  return net::ERR_IO_PENDING;
}

void BraveShieldsCore::OnURLRequestDestroyed(uint64_t request_id) {
  if (ContainsKey(callbacks_, request_id)) {
    callbacks_.erase(request_id);
  }
}

void BraveShieldsCore::RunCallbackForRequestIdentifier(
    uint64_t request_identifier,
    int rv) {
  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(request_identifier);
  // TODO(iefremov): Double check this.
  //std::move(it->second).Run(rv);
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::BindOnce(std::move(it->second), rv));
}

void BraveShieldsCore::RunNextCallback(
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (!ContainsKey(callbacks_, ctx->request_identifier)) {
    LOG(ERROR) << "Exiting " << ctx->request_identifier;
    return;
  }

  // Continue processing callbacks until we hit one that returns PENDING
  int rv = net::OK;

  if (ctx->event_type == brave::kOnBeforeRequest) {
    while (before_url_request_callbacks_.size() !=
           ctx->next_url_request_index) {
      brave::OnBeforeURLRequestCallback callback =
          before_url_request_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveShieldsCore::RunNextCallback,
                     base::Unretained(this), ctx);
      rv = callback.Run(next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    while (before_start_transaction_callbacks_.size() !=
           ctx->next_url_request_index) {
      brave::OnBeforeStartTransactionCallback callback =
          before_start_transaction_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveShieldsCore::RunNextCallback,
                     base::Unretained(this), ctx);
      rv = callback.Run(ctx->headers, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    while (headers_received_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnHeadersReceivedCallback callback =
          headers_received_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveShieldsCore::RunNextCallback,
                     base::Unretained(this), ctx);
      rv = callback.Run(ctx->original_response_headers,
                        ctx->override_response_headers,
                        ctx->allowed_unsafe_redirect_url, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  }

  if (rv != net::OK) {
    RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
    return;
  }

  net::CompletionOnceCallback wrapped_callback =
      base::BindOnce(&BraveShieldsCore::RunCallbackForRequestIdentifier,
                     base::Unretained(this), ctx->request_identifier);

  if (ctx->event_type == brave::kOnBeforeRequest) {
    if (!ctx->new_url_spec.empty() &&
        (ctx->new_url_spec != ctx->request_url.spec()) &&
        IsRequestIdentifierValid(ctx->request_identifier)) {
      *ctx->new_url = GURL(ctx->new_url_spec);
    }
    if (ctx->blocked_by == brave::kAdBlocked ||
        ctx->blocked_by == brave::kTrackerBlocked) {
      if (ctx->cancel_request_explicitly) {
        RunCallbackForRequestIdentifier(ctx->request_identifier,
            net::ERR_ABORTED);
        return;
      }
    }
  }
  // ChromeNetworkDelegate returns net::ERR_IO_PENDING if an extension is
  // intercepting the request and OK if the request should proceed normally.
  if (rv != net::ERR_IO_PENDING) {
    RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
  }
}

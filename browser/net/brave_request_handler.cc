/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_request_handler.h"

#include <algorithm>
#include <utility>

#include "base/task/post_task.h"
#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_httpse_network_delegate_helper.h"
#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"
#include "brave/browser/net/brave_stp_util.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/browser/net/brave_referrals_network_delegate_helper.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#include "brave/browser/net/brave_translate_redirect_network_delegate_helper.h"
#endif

BraveRequestHandler::BraveRequestHandler()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()) {
  SetupCallbacks();
  // Initialize the preference change registrar.
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::Bind(&BraveRequestHandler::InitPrefChangeRegistrarOnUI,
                 weak_factory_.GetWeakPtr()));
}

BraveRequestHandler::~BraveRequestHandler() = default;

void BraveRequestHandler::SetupCallbacks() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  brave::OnBeforeURLRequestCallback callback =
      base::Bind(brave::OnBeforeURLRequest_SiteHacksWork);
  before_url_request_callbacks_.push_back(callback);

  callback = base::Bind(brave::OnBeforeURLRequest_AdBlockTPPreWork);
  before_url_request_callbacks_.push_back(callback);

  callback = base::Bind(brave::OnBeforeURLRequest_HttpsePreFileWork);
  before_url_request_callbacks_.push_back(callback);

  callback = base::Bind(brave::OnBeforeURLRequest_CommonStaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  callback = base::Bind(brave_rewards::OnBeforeURLRequest);
  before_url_request_callbacks_.push_back(callback);
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  callback =
      base::BindRepeating(brave::OnBeforeURLRequest_TranslateRedirectWork);
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
      base::Bind(webtorrent::OnHeadersReceived_TorrentRedirectWork);
  headers_received_callbacks_.push_back(headers_received_callback);
#endif
}

void BraveRequestHandler::InitPrefChangeRegistrarOnUI() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  PrefService* prefs = g_browser_process->local_state();
  pref_change_registrar_.reset(new PrefChangeRegistrar());
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(
      kReferralHeaders,
      base::Bind(&BraveRequestHandler::OnReferralHeadersChanged,
                 weak_factory_ui_.GetWeakPtr()));
  // Retrieve current referral headers, if any.
  OnReferralHeadersChanged();
#endif
}

void BraveRequestHandler::OnReferralHeadersChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (const base::ListValue* referral_headers =
          g_browser_process->local_state()->GetList(kReferralHeaders)) {
    task_runner_->PostTask(FROM_HERE,
        base::Bind(&BraveRequestHandler::SetReferralHeaders,
                   weak_factory_ui_.GetWeakPtr(),
                   referral_headers->DeepCopy()));
  }
}

void BraveRequestHandler::SetReferralHeaders(
    base::ListValue* referral_headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  referral_headers_list_.reset(referral_headers);
}

int BraveRequestHandler::OnBeforeURLRequest(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (before_url_request_callbacks_.empty()) {
    return net::OK;
  }
  ctx->new_url = new_url;
  ctx->event_type = brave::kOnBeforeRequest;
  callbacks_[ctx->request_identifier] = std::move(callback);
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::Bind(&BraveRequestHandler::RunNextCallback,
                                      weak_factory_.GetWeakPtr(),
                                      ctx));
  return net::ERR_IO_PENDING;
}

int BraveRequestHandler::OnBeforeStartTransaction(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (before_start_transaction_callbacks_.empty()) {
    return net::OK;
  }
  ctx->event_type = brave::kOnBeforeStartTransaction;
  ctx->headers = headers;
  ctx->referral_headers_list = referral_headers_list_.get();
  callbacks_[ctx->request_identifier] = std::move(callback);
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::Bind(&BraveRequestHandler::RunNextCallback,
                                      weak_factory_.GetWeakPtr(),
                                      ctx));
  return net::ERR_IO_PENDING;
}

int BraveRequestHandler::OnHeadersReceived(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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

  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::Bind(&BraveRequestHandler::RunNextCallback,
                                      weak_factory_.GetWeakPtr(),
                                      ctx));
  return net::ERR_IO_PENDING;
}

void BraveRequestHandler::OnURLRequestDestroyed(
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (base::Contains(callbacks_, ctx->request_identifier)) {
    callbacks_.erase(ctx->request_identifier);
  }
}

void BraveRequestHandler::RunCallbackForRequestIdentifier(
    uint64_t request_identifier,
    int rv) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(
      &BraveRequestHandler::RunCallbackForRequestIdentifierInTaskRunner,
      weak_factory_io_.GetWeakPtr(), request_identifier, rv));
}

void BraveRequestHandler::RunCallbackForRequestIdentifierInTaskRunner(
    uint64_t request_identifier,
    int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::Contains(callbacks_, request_identifier))
    return;

  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(request_identifier);
  // We intentionally do the async call to maintain the proper flow
  // of URLLoader callbacks.
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(it->second), rv));
}

// TODO(iefremov): Merge all callback containers into one and run only one loop
// instead of many (issues/5574).
void BraveRequestHandler::RunNextCallback(
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Continue processing callbacks until we hit one that returns PENDING
  int rv = net::OK;

  if (ctx->event_type == brave::kOnBeforeRequest) {
    while (before_url_request_callbacks_.size() !=
           ctx->next_url_request_index) {
      brave::OnBeforeURLRequestCallback callback =
          before_url_request_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback = base::Bind(
          &BraveRequestHandler::RunNextCallback,
          weak_factory_.GetWeakPtr(),
          ctx);
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
      brave::ResponseCallback next_callback = base::Bind(
          &BraveRequestHandler::RunNextCallback,
          weak_factory_.GetWeakPtr(),
          ctx);
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
      brave::ResponseCallback next_callback = base::Bind(
          &BraveRequestHandler::RunNextCallback,
          weak_factory_.GetWeakPtr(),
          ctx);
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

  if (ctx->event_type == brave::kOnBeforeRequest) {
    if (!ctx->new_url_spec.empty() &&
        ctx->new_url_spec != ctx->request_url.spec()) {
      *ctx->new_url = GURL(ctx->new_url_spec);
    }
    if (ctx->blocked_by == brave::kAdBlocked) {
      if (ctx->cancel_request_explicitly) {
        RunCallbackForRequestIdentifier(ctx->request_identifier,
                                        net::ERR_ABORTED);
        return;
      }
    }
  }
  RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_BASE_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_BASE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/strings/string_piece.h"
#include "brave/browser/net/url_context.h"
#include "chrome/browser/net/chrome_network_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/completion_once_callback.h"

class PrefChangeRegistrar;

namespace extensions {
class EventRouterForwarder;
}

namespace net {
class URLRequest;
}

base::flat_set<base::StringPiece>* TrackableSecurityHeaders();

void RemoveTrackableSecurityHeadersForThirdParty(
    net::URLRequest* request,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers);

// BraveNetworkDelegateBase is the central point from within the Brave code to
// add hooks into the network stack.
class BraveNetworkDelegateBase : public ChromeNetworkDelegate {
 public:
  using ResponseCallback = base::Callback<void(const base::DictionaryValue&)>;
  using ResponseListener = base::Callback<void(const base::DictionaryValue&,
                                               const ResponseCallback&)>;

  explicit BraveNetworkDelegateBase(
      extensions::EventRouterForwarder* event_router);
  ~BraveNetworkDelegateBase() override;

  bool IsRequestIdentifierValid(uint64_t request_identifier);

  // NetworkDelegate implementation.
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;

  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list,
                       bool allowed_from_caller) override;

  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      net::CookieOptions* options,
                      bool allowed_from_caller) override;

  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void RunCallbackForRequestIdentifier(uint64_t request_identifier, int rv);

 protected:
  void RunNextCallback(net::URLRequest* request,
                       std::shared_ptr<brave::BraveRequestInfo> ctx);
  std::vector<brave::OnBeforeURLRequestCallback> before_url_request_callbacks_;
  std::vector<brave::OnBeforeStartTransactionCallback>
      before_start_transaction_callbacks_;
  std::vector<brave::OnHeadersReceivedCallback> headers_received_callbacks_;
  std::vector<brave::OnCanGetCookiesCallback> can_get_cookies_callbacks_;
  std::vector<brave::OnCanSetCookiesCallback> can_set_cookies_callbacks_;

 private:
  void InitPrefChangeRegistrarOnUI();
  void SetReferralHeaders(base::ListValue* referral_headers);
  void OnReferralHeadersChanged();
  void OnPreferenceChanged(const std::string& pref_name);
  void UpdateAdBlockFromPref(const std::string& pref_name);

  // TODO(iefremov): actually, we don't have to keep the list here, since
  // it is global for the whole browser and could live a singletonce in the
  // rewards service. Eliminating this will also help to avoid using
  // PrefChangeRegistrar and corresponding |base::Unretained| usages, that are
  // illegal.
  std::unique_ptr<base::ListValue> referral_headers_list_;
  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;
  std::unique_ptr<PrefChangeRegistrar, content::BrowserThread::DeleteOnUIThread>
      pref_change_registrar_;
  std::unique_ptr<PrefChangeRegistrar, content::BrowserThread::DeleteOnUIThread>
      user_pref_change_registrar_;

  bool allow_google_auth_;

  DISALLOW_COPY_AND_ASSIGN(BraveNetworkDelegateBase);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_BASE_H_

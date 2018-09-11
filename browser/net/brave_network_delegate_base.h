/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_BASE_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_BASE_H_

#include "brave/browser/net/url_context.h"
#include "chrome/browser/net/chrome_network_delegate.h"
#include "net/base/completion_callback.h"

namespace extensions {
class EventRouterForwarder;
}

namespace net {
class URLRequest;
}

// BraveNetworkDelegateBase is the central point from within the Brave code to
// add hooks into the network stack.
class BraveNetworkDelegateBase : public ChromeNetworkDelegate {
 public:

  using ResponseCallback = base::Callback<void(const base::DictionaryValue&)>;
  using ResponseListener = base::Callback<void(const base::DictionaryValue&,
                                               const ResponseCallback&)>;

  BraveNetworkDelegateBase(extensions::EventRouterForwarder* event_router);
  ~BraveNetworkDelegateBase() override;

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

  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void RunCallbackForRequestIdentifier(uint64_t request_identifier, int rv);

 protected:
  void RunNextCallback(
    net::URLRequest* request,
    GURL *new_url,
    std::shared_ptr<brave::BraveRequestInfo> ctx);
  std::vector<brave::OnBeforeURLRequestCallback>
      before_url_request_callbacks_;
  std::vector<brave::OnBeforeStartTransactionCallback>
      before_start_transaction_callbacks_;
  std::vector<brave::OnHeadersReceivedCallback>
      headers_received_callbacks_;

 private:
  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(BraveNetworkDelegateBase);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_BASE_H_

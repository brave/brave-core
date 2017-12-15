/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_H_

#include "chrome/browser/net/chrome_network_delegate.h"
#include "brave/browser/net/url_context.h"

template<class T> class PrefMember;

typedef PrefMember<bool> BooleanPrefMember;

namespace extensions {
class EventRouterForwarder;
}

namespace net {
class URLRequest;
}

// BraveNetworkDelegate is the central point from within the Brave code to
// add hooks into the network stack.
class BraveNetworkDelegate : public ChromeNetworkDelegate {
 public:

  using ResponseCallback = base::Callback<void(const base::DictionaryValue&)>;
  using ResponseListener = base::Callback<void(const base::DictionaryValue&,
                                               const ResponseCallback&)>;

  enum ResponseEvent {
    kOnBeforeRequest,
    kOnBeforeSendHeaders,
    kOnHeadersReceived,
  };

  struct ResponseListenerInfo {
    ResponseListenerInfo();
    ~ResponseListenerInfo();

    ResponseListener listener_;
  };

  // |enable_referrers| (and all of the other optional PrefMembers) should be
  // initialized on the UI thread (see below) beforehand. This object's owner is
  // responsible for cleaning them up at shutdown.
  BraveNetworkDelegate(extensions::EventRouterForwarder* event_router,
                        BooleanPrefMember* enable_referrers);
  ~BraveNetworkDelegate() override;

  // NetworkDelegate implementation.
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;

 protected:
  void RunNextCallback(
    net::URLRequest* request,
    GURL *new_url,
    std::shared_ptr<brave::OnBeforeURLRequestContext> ctx);

 private:
  std::vector<brave::OnBeforeURLRequestCallback>
      before_url_request_callbacks_;
  std::map<ResponseEvent, ResponseListenerInfo> response_listeners_;
  std::map<uint64_t, net::CompletionCallback> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(BraveNetworkDelegate);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_H_

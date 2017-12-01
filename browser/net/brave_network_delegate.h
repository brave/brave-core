/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_H_

#include "chrome/browser/net/chrome_network_delegate.h"

template<class T> class PrefMember;

typedef PrefMember<bool> BooleanPrefMember;
struct OnBeforeURLRequestContext;
class PendingRequests;

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

 protected:
  int OnBeforeURLRequest_HttpsePreFileWork(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      GURL* new_url,
      std::shared_ptr<OnBeforeURLRequestContext> ctx);
  void OnBeforeURLRequest_HttpseFileWork(
      net::URLRequest* request,
      std::shared_ptr<OnBeforeURLRequestContext> ctx);
  int OnBeforeURLRequest_HttpsePostFileWork(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      GURL* new_url,
      std::shared_ptr<OnBeforeURLRequestContext> ctx);
  bool PendedRequestIsDestroyedOrCancelled(
            OnBeforeURLRequestContext* ctx,
            net::URLRequest* request);
  // (TODO)find a better way to handle last first party
  // This is a hack from Android
  GURL last_first_party_url_;
  std::auto_ptr<PendingRequests> pending_requests_;
  DISALLOW_COPY_AND_ASSIGN(BraveNetworkDelegate);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_DELEGATE_H_

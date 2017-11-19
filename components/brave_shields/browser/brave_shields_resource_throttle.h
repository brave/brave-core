/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_LOADER_BRAVE_SHIELDS_RESOURCE_THROTTLE_H_
#define BRAVE_BROWSER_LOADER_BRAVE_SHIELDS_RESOURCE_THROTTLE_H_

#include "base/macros.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_type.h"

namespace net {
struct RedirectInfo;
class URLRequest;
}

namespace content {
class ResourceContext;
}

// Contructs a resource throttle for Brave shields like tracking protection
// and adblock. It returns a
content::ResourceThrottle* MaybeCreateBraveShieldsResourceThrottle(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::ResourceType resource_type);

// This check is done before requesting the original URL, and additionally
// before following any subsequent redirect.
class BraveShieldsResourceThrottle
    : public content::ResourceThrottle {
 private:
  friend content::ResourceThrottle* MaybeCreateBraveShieldsResourceThrottle(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::ResourceType resource_type);

  BraveShieldsResourceThrottle(const net::URLRequest* request,
                               content::ResourceContext* resource_context,
                               content::ResourceType resource_type);

  ~BraveShieldsResourceThrottle() override;

  // content::ResourceThrottle:
  void WillStartRequest(bool* defer) override;
  const char* GetNameForLogging() const override;

  const net::URLRequest* request_;
  content::ResourceContext* resource_context_;
  content::ResourceType resource_type_;

  DISALLOW_COPY_AND_ASSIGN(BraveShieldsResourceThrottle);
};

#endif  // BRAVE_BROWSER_LOADER_BRAVE_SHIELDS_RESOURCE_THROTTLE_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_NET_IOS_CHROME_NETWORK_DELEGATE_H__
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_NET_IOS_CHROME_NETWORK_DELEGATE_H__

class IOSChromeNetworkDelegate;
using IOSChromeNetworkDelegate_BraveImpl = IOSChromeNetworkDelegate;

#define IOSChromeNetworkDelegate IOSChromeNetworkDelegate_ChromiumImpl
#include "src/ios/chrome/browser/net/ios_chrome_network_delegate.h"
#undef IOSChromeNetworkDelegate

class IOSChromeNetworkDelegate : public IOSChromeNetworkDelegate_ChromiumImpl {
 public:
  using IOSChromeNetworkDelegate_ChromiumImpl::
      IOSChromeNetworkDelegate_ChromiumImpl;

  IOSChromeNetworkDelegate(const IOSChromeNetworkDelegate&) = delete;
  IOSChromeNetworkDelegate& operator=(const IOSChromeNetworkDelegate&) = delete;

  ~IOSChromeNetworkDelegate() override;

 private:
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override;
};

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_NET_IOS_CHROME_NETWORK_DELEGATE_H__

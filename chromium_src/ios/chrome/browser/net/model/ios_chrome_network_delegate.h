/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_NET_MODEL_IOS_CHROME_NETWORK_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_NET_MODEL_IOS_CHROME_NETWORK_DELEGATE_H_

class IOSChromeNetworkDelegate;
using IOSChromeNetworkDelegate_BraveImpl = IOSChromeNetworkDelegate;

#define IOSChromeNetworkDelegate IOSChromeNetworkDelegate_ChromiumImpl
#define BRAVE_IOS_CHROME_NETWORK_DELEGATE_H \
  friend IOSChromeNetworkDelegate_BraveImpl;

#include "src/ios/chrome/browser/net/model/ios_chrome_network_delegate.h"  // IWYU pragma: export
#undef IOSChromeNetworkDelegate
#undef BRAVE_IOS_CHROME_NETWORK_DELEGATE_H

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

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_NET_MODEL_IOS_CHROME_NETWORK_DELEGATE_H_

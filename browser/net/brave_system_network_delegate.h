/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_SYSTEM_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_SYSTEM_NETWORK_DELEGATE_H_

#include "brave/browser/net/brave_network_delegate_base.h"

class BraveSystemNetworkDelegate : public BraveNetworkDelegateBase {
 public:
  BraveSystemNetworkDelegate(extensions::EventRouterForwarder* event_router);
  ~BraveSystemNetworkDelegate() override;

  DISALLOW_COPY_AND_ASSIGN(BraveSystemNetworkDelegate);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_SYSTEM_NETWORK_DELEGATE_H_

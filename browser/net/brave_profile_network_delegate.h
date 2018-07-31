/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_PROFILE_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_PROFILE_NETWORK_DELEGATE_H_

#include "brave/browser/net/brave_network_delegate_base.h"

class BraveProfileNetworkDelegate : public BraveNetworkDelegateBase {
 public:
  BraveProfileNetworkDelegate(extensions::EventRouterForwarder* event_router);
  ~BraveProfileNetworkDelegate() override;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileNetworkDelegate);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_PROFILE_NETWORK_DELEGATE_H_

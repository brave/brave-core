// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/web/web_state/web_state.mm"

namespace web {
bool WebState::InterfaceBinder::HasUntrustedInterface(
    const GURL& url,
    const std::string& interface_name) {
  DCHECK(!url.is_empty());
  DCHECK(!interface_name.empty());
  if (auto it = untrusted_callbacks_.find(url);
      it != untrusted_callbacks_.end()) {
    return it->second.find(interface_name) != it->second.end();
  }
  return false;
}

void WebState::InterfaceBinder::BindUntrustedInterface(
    const GURL& url,
    mojo::GenericPendingReceiver receiver) {
  DCHECK(!url.is_empty());
  DCHECK(receiver.is_valid());
  if (auto it = untrusted_callbacks_.find(url);
      it != untrusted_callbacks_.end()) {
    if (auto jt = it->second.find(*receiver.interface_name());
        jt != it->second.end()) {
      jt->second.Run(&receiver);

      GetWebClient()->BindInterfaceReceiverFromMainFrame(web_state_,
                                                         std::move(receiver));
    }
  }
}
}  // namespace web

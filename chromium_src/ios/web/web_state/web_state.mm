// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/check.h"

#include <ios/web/web_state/web_state.mm>

namespace web {
bool WebState::InterfaceBinder::IsAllowedForOrigin(
    const GURL& origin,
    std::string_view interface_name) {
  DCHECK(!origin.is_empty() && origin.is_valid());
  DCHECK(!interface_name.empty());
  if (auto it = untrusted_callbacks_.find(origin.host());
      it != untrusted_callbacks_.end()) {
    return it->second.count(interface_name);
  }
  return false;
}

void WebState::InterfaceBinder::RemoveUntrustedInterface(
    const GURL& origin,
    std::string_view interface_name) {
  if (auto it = untrusted_callbacks_.find(origin.host());
      it != untrusted_callbacks_.end()) {
    if (auto jt = it->second.find(interface_name); jt != it->second.end()) {
      it->second.erase(jt);

      RemoveInterface(interface_name);
    }

    if (it->second.empty()) {
      untrusted_callbacks_.erase(it);
    }
  }
}
}  // namespace web

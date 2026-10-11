// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/web_state/web_state_impl_realized_web_state.mm>

namespace web {

// The first WebUI created should always be the main frame's.
WebUIIOS* WebStateImpl::RealizedWebState::GetMainFrameWebUI() {
  if (web_uis_.empty()) {
    return nullptr;
  }
  return web_uis_.begin()->second.get();
}

size_t WebStateImpl::RealizedWebState::GetWebUICountForTesting() {
  return web_uis_.size();
}

}  // namespace web

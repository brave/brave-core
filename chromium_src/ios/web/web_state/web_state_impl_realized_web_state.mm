// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/web_state/web_state_impl_realized_web_state.h"

// Support for replacing the following methods below
#define ClearWebUI ClearWebUI_ChromiumImpl
#define CreateWebUI(url) CreateWebUI_ChromiumImpl(url)
#define HandleWebUIMessage HandleWebUIMessage_ChromiumImpl
#define HasWebUI HasWebUI_ChromiumImpl
#define TearDown TearDown_ChromiumImpl

#include <ios/web/web_state/web_state_impl_realized_web_state.mm>

#undef TearDown
#undef HasWebUI
#undef HandleWebUIMessage
#undef CreateWebUI
#undef ClearWebUI

namespace web {

// These new implementations mimic their non-Brave counterparts in
// web_state_impl_realized_web_state.mm but store and fetch from the newly added
// map instead of the `web_ui_` field
//
// See web_state_impl_realized_web_state.h for reasons why we are replacing
// these implementations

void WebStateImpl::RealizedWebState::TearDown() {
  // Call the original ClearWebUI which would have been replaced with
  // ClearWebUI_ChromiumImpl
  ClearWebUI();
  TearDown_ChromiumImpl();
}

void WebStateImpl::RealizedWebState::CreateWebUI(const GURL& url) {
  const std::string_view host = url.host();
  if (web_uis_.contains(host)) {
    // Don't recreate WebUI for the same host. At the moment this is a required
    // limitation as we don't have the neccessary info to associate a WebUI
    // with a specific frame like desktop/android does.
    return;
  }
  auto web_ui = CreateWebUIIOS(url);
  if (web_ui) {
    web_uis_.insert({std::string(host), std::move(web_ui)});
  }
}

void WebStateImpl::RealizedWebState::ClearWebUI() {
  web_uis_.clear();
}

void WebStateImpl::RealizedWebState::HandleWebUIMessage(
    const GURL& source_url,
    std::string_view message,
    const base::Value::List& args) {
  const std::string_view host = source_url.host();
  auto web_ui = web_uis_.find(host);
  if (web_ui != web_uis_.end() && web_ui->second) {
    web_ui->second->ProcessWebUIIOSMessage(source_url, message, args);
  }
}

bool WebStateImpl::RealizedWebState::HasWebUI() const {
  return !web_uis_.empty();
}

// Implements the new method we expose to get the main frame's WebUI
WebUIIOS* WebStateImpl::RealizedWebState::GetMainFrameWebUI() {
  if (web_uis_.empty()) {
    return nullptr;
  }
  // The first WebUI created should always be the main frame
  return web_uis_.begin()->second.get();
}

size_t WebStateImpl::RealizedWebState::GetWebUICountForTesting() {
  return web_uis_.size();
}

}  // namespace web

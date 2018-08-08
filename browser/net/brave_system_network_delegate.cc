/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_network_delegate.h"

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

BraveSystemNetworkDelegate::BraveSystemNetworkDelegate(
    extensions::EventRouterForwarder* event_router) :
    BraveNetworkDelegateBase(event_router) {
  brave::OnBeforeURLRequestCallback callback =
      base::Bind(
          brave::OnBeforeURLRequest_StaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);
  callback = base::Bind(
          brave::OnBeforeURLRequest_CommonStaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);
}

BraveSystemNetworkDelegate::~BraveSystemNetworkDelegate() {
}

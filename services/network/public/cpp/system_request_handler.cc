/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/services/network/public/cpp/system_request_handler.h"

namespace network {

SystemRequestHandler* SystemRequestHandler::GetInstance() {
  return base::Singleton<SystemRequestHandler>::get();
}

void SystemRequestHandler::RegisterOnBeforeSystemRequestCallback(
    const OnBeforeSystemRequestCallback& cb) {
  on_before_system_request_callback_ = cb;
}

network::ResourceRequest SystemRequestHandler::OnBeforeSystemRequest(
    const network::ResourceRequest& url_request) {
  if (!on_before_system_request_callback_) {
    NOTREACHED();
    return url_request;
  }
  return on_before_system_request_callback_.Run(url_request);
}

SystemRequestHandler::SystemRequestHandler() {}

SystemRequestHandler::~SystemRequestHandler() {}

}  // namespace network

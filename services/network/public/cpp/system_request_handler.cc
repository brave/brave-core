/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/services/network/public/cpp/system_request_handler.h"
#include "base/no_destructor.h"

namespace network {

SystemRequestHandler* SystemRequestHandler::GetInstance() {
  static base::NoDestructor<SystemRequestHandler> instance;
  return instance.get();
}

void SystemRequestHandler::RegisterOnBeforeSystemRequestCallback(
    const OnBeforeSystemRequestCallback& cb) {
  on_before_system_request_callback_ = cb;
}

network::ResourceRequest SystemRequestHandler::OnBeforeSystemRequest(
    const network::ResourceRequest& url_request) {
  if (!on_before_system_request_callback_) {
    // Changing to LOG(ERROR) to avoid crash dump uploading as this is spamming
    // our Backtrace system at the moment. Generally, if we get here, it means
    // that `BraveBrowserProcessImpl::Init` hasn't been called yet and so we
    // don't need to apply our filters in this case.
    LOG(ERROR) << "SystemRequestHandler::OnBeforeSystemRequest called before "
                  "BraveBrowserProcessImpl::Init";
    return url_request;
  }
  return on_before_system_request_callback_.Run(url_request);
}

SystemRequestHandler::SystemRequestHandler() = default;

SystemRequestHandler::~SystemRequestHandler() = default;

}  // namespace network

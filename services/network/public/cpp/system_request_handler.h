/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_SERVICES_NETWORK_PUBLIC_CPP_SYSTEM_REQUEST_HANDLER_H_
#define BRAVE_SERVICES_NETWORK_PUBLIC_CPP_SYSTEM_REQUEST_HANDLER_H_

#include "base/callback.h"
#include "base/component_export.h"
#include "base/memory/singleton.h"
#include "services/network/public/cpp/resource_request.h"

namespace network {

class COMPONENT_EXPORT(NETWORK_CPP) SystemRequestHandler {
 public:
  typedef base::RepeatingCallback<network::ResourceRequest(
      const network::ResourceRequest&)>
      OnBeforeSystemRequestCallback;

  SystemRequestHandler(const SystemRequestHandler&) = delete;
  SystemRequestHandler& operator=(const SystemRequestHandler&) = delete;

  // Returns the instance of SystemRequestHandler.
  static SystemRequestHandler* GetInstance();

  // Registers callback to be invoked in different layers.
  void RegisterOnBeforeSystemRequestCallback(
      const OnBeforeSystemRequestCallback& cb);

  network::ResourceRequest OnBeforeSystemRequest(
      const network::ResourceRequest& url_request);

 private:
  friend struct base::DefaultSingletonTraits<SystemRequestHandler>;

  SystemRequestHandler();
  ~SystemRequestHandler();

  OnBeforeSystemRequestCallback on_before_system_request_callback_;
};

}  // namespace network

#endif  // BRAVE_SERVICES_NETWORK_PUBLIC_CPP_SYSTEM_REQUEST_HANDLER_H_

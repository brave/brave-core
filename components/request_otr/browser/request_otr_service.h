/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_SERVICE_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class PrefRegistrySimple;

namespace request_otr {

class RequestOTRComponentInstaller;

class RequestOTRService : public KeyedService {
 public:
  enum class RequestOTRActionOption {
    kAsk = 0,
    kAlways,
    kNever,
  };

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  explicit RequestOTRService(RequestOTRComponentInstaller* component_installer);
  RequestOTRService(const RequestOTRService&) = delete;
  RequestOTRService& operator=(const RequestOTRService&) = delete;
  ~RequestOTRService() override;

  bool ShouldBlock(const GURL& url) const;

 private:
  RequestOTRComponentInstaller* component_installer_ = nullptr;  // NOT OWNED
  base::WeakPtrFactory<RequestOTRService> weak_factory_{this};
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_SERVICE_H_

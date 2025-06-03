/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

class BraveAccountService : public KeyedService {
 public:
  explicit BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveAccountService() override;

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

 private:
  const raw_ptr<PrefService> pref_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

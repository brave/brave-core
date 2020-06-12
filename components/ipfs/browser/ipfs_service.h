/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

class IpfsService : public KeyedService {
 public:
  explicit IpfsService(content::BrowserContext* context);
  ~IpfsService() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(IpfsService);
};

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_H_

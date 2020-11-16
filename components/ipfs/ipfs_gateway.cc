/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_gateway.h"

#include "base/logging.h"
#include "brave/components/ipfs/ipfs_constants.h"

namespace ipfs {

GURL ipfs_default_gateway_for_test;

void SetIPFSDefaultGatewayForTest(const GURL& url) {
  ipfs_default_gateway_for_test = url;
}

GURL GetDefaultIPFSLocalGateway() {
  return GURL(kDefaultIPFSLocalGateway);
}

GURL GetDefaultIPFSGateway() {
  if (!ipfs_default_gateway_for_test.is_empty()) {
    return GURL(ipfs_default_gateway_for_test);
  }
  return GURL(kDefaultIPFSGateway);
}

}  // namespace ipfs

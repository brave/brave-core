/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_NODE_TRAFFIC_RECOGNIZER_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_NODE_TRAFFIC_RECOGNIZER_H_

#include "components/version_info/channel.h"
#include "url/gurl.h"

namespace ipfs {

class IpfsNodeTrafficRecognizer {
 public:
  static bool IsKuboRelatedPort(const GURL& request_url,
                                const version_info::Channel& channel);
  static bool IsKuboRelatedDomain(const GURL& request_url);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_NODE_TRAFFIC_RECOGNIZER_H_

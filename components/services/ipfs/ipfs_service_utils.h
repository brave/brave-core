/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_IPFS_IPFS_SERVICE_UTILS_H_
#define BRAVE_COMPONENTS_SERVICES_IPFS_IPFS_SERVICE_UTILS_H_

#include <string>

namespace ipfs {

namespace mojom {
class IpfsConfig;
}

// Updates the ipfs node config to meet current preferences
bool UpdateConfigJSON(const std::string& source,
                      const ipfs::mojom::IpfsConfig* config,
                      std::string* result);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_SERVICES_IPFS_IPFS_SERVICE_UTILS_H_

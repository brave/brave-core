/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

#include <string>

#include "base/files/file_path.h"
#include "url/gurl.h"

namespace ipfs {

inline constexpr char kIPFSScheme[] = "ipfs";
inline constexpr char kDefaultPublicGateway[] = "https://ipfs.io";

bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      bool use_subdomain);

GURL ContentHashToCIDv1URL(base::span<const uint8_t> contenthash);

std::string GetIpfsClientComponentId();

void DeleteIpfsComponentAndData(const base::FilePath& user_data_dir,
                                const std::string& ipfs_client_component_id);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

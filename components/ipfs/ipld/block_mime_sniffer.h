/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_MIME_SNIFFER_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_MIME_SNIFFER_H_

#include <string>
#include "absl/types/optional.h"
#include "url/gurl.h"

namespace ipfs::ipld {

class BlockMimeSniffer {
 public:
  BlockMimeSniffer();
  ~BlockMimeSniffer();

  absl::optional<std::string> GetMime(const std::string& file_extension,
                      std::string_view content,
                      const GURL& url);
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_MIME_SNIFFER_H_

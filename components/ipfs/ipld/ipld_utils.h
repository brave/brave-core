/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_IPLD_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_IPLD_UTILS_H_

#include "brave/components/ipfs/rs/src/lib.rs.h"

namespace ipfs::ipld {

CarV1HeaderResult DecodeCarv1Header(const std::vector<uint8_t>& data);

CarV2HeaderResult DecodeCarv2Header(const std::vector<uint8_t>& data);

BlockDecodeResult DecodeBlockInfo(const uint64_t& offset,
                                    const std::vector<uint8_t>& data);

BlockContentDecodeResult DecodeBlockContent(const uint64_t& offset,
                                    const std::vector<uint8_t>& data);
}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_IPLD_UTILS_H_
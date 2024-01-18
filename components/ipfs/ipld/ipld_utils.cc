/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/ipld_utils.h"
#include "brave/components/ipfs/rs/src/lib.rs.h"

namespace ipfs::ipld {

CarV1HeaderResult DecodeCarv1Header(const std::vector<uint8_t>& data) {
  return ipfs::decode_carv1_header(data);
}

CarV2HeaderResult DecodeCarv2Header(const std::vector<uint8_t>& data) {
  return ipfs::decode_carv2_header(data);
}

BlockDecodeResult DecodeBlockInfo(const uint64_t& offset,
                                    const std::vector<uint8_t>& data) {
  return ipfs::decode_block_info(offset, data);
}

BlockContentDecodeResult DecodeBlockContent(const uint64_t& offset,
                                    const std::vector<uint8_t>& data) {
  return ipfs::decode_block_content(offset, data); 
}

}  // namespace ipfs::ipld

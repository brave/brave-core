/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/rs/src/lib.rs.h"

namespace ipfs::ipld {

CarV1HeaderResult decode_carv1_header(const std::vector<uint8_t>& data);

CarV2HeaderResult decode_carv2_header(const std::vector<uint8_t>& data);

BlockDecodeResult decode(const uint64_t& offset, const std::vector<uint8_t>& data);
}
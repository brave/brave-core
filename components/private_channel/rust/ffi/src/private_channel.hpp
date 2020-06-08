/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "lib.h"
}

namespace private_channel {

	C_ResultChallenge start_challenge(
    const char** input_ptr, int size, const uint8_t* server_pk);
	
  C_ResultSecondRound second_round(
    const uint8_t* enc_input_ptr, int size,  const uint8_t*  sk);
}

#endif	// BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_

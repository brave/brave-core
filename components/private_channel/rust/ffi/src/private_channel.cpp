/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/private_channel/rust/ffi/src/private_channel.hpp"
#include <iostream>

extern "C" {
#include "brave/components/private_channel/rust/ffi/src/lib.h"
}

namespace private_channel {

  C_ResultChallenge start_challenge(
    const char** input_ptr, int size, const uint8_t* server_pk) {
      return client_start_challenge(input_ptr, size, server_pk);
  }

  C_ResultSecondRound second_round(
    const uint8_t* enc_input_ptr, int input_size, const uint8_t*  sk) {
      return client_second_round(enc_input_ptr, input_size, sk);
  }

}  // namespace private_channel

#ifndef PRIVATE_CHANNEL_RUST_FFI_SRC_WRAPPER
#define PRIVATE_CHANNEL_RUST_FFI_SRC_WRAPPER
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "lib.h"
}

namespace private_channel {

	C_ResultChallenge start_challenge(const char** input_ptr, int size, const uint8_t* server_pk);
	C_ResultSecondRound second_round(const uint8_t* enc_input_ptr, int size,  const uint8_t*  sk);
}

#endif	// PRIVATE_CHANNEL_RUST_FFI_SRC_WRAPPER

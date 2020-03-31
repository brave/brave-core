#include "client_attestation.hpp"
#include <iostream>
using namespace std;

extern "C" {
#include "lib.h"
}

namespace client_attestation {

	C_ResultChallenge start_challenge(const char** input_ptr, int size, const uint8_t* server_pk) {
		return client_start_challenge(input_ptr, size, server_pk);
	}

	C_ResultSecondRound second_round(const uint8_t* enc_input_ptr, int input_size, const uint8_t*  sk) {
		return client_second_round(enc_input_ptr, input_size, sk);
	}

}	// namespace client_attestation

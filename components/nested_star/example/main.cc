/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "lib.rs.h"  // NOLINT

int main(int argc, char** argv) {
  uint8_t epoch = 1;
  std::vector<std::string> input = {"TestMetricOne|1", "TestMetricTwo|2"};
  std::string output;

  auto public_key = nested_star::get_ppoprf_null_public_key();

  auto rrs_res = nested_star::prepare_measurement(input, epoch);
  if (rrs_res.error.size() > 0) {
    std::cerr << "Error preparing measurement: " << rrs_res.error.c_str()
              << std::endl;
    return 1;
  }
  auto req = nested_star::construct_randomness_request(*rrs_res.state);

  auto rand_resp = nested_star::generate_local_randomness(req, epoch);
  if (rand_resp.error.size() > 0) {
    std::cerr << "Error generating local randomness: "
              << rand_resp.error.c_str() << std::endl;
    return 3;
  }

  auto msg_res = construct_message(rand_resp.points, rand_resp.proofs,
                                   *rrs_res.state, *public_key, {}, 50);
  if (msg_res.error.size() > 0) {
    std::cerr << "Error generating final message: " << msg_res.error.c_str()
              << std::endl;
    return 4;
  }

  std::cout << "STAR message (hex): " << std::hex;
  for (auto b : msg_res.data) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << int(b);
  }
  std::cout << std::endl;
}

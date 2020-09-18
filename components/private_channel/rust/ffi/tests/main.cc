/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "private_channel.h"  // NOLINT

#define KEY_SIZE 32

char const* SERVER_PK =
    "[78, 181, 75, 245, 70, 218, 146, 152, 155, 118, 20, 184, 203, 179, 192, "
    "222, 212, 79, 178, 76, 232, 250, 218, 196, 6, 254, 139, 145, 172, 18, "
    "189, 13]";  // NOLINT

char const* SERVER_PK_MALFORMED =
    "[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 179, 192, 222, 212, 79, 178, 76, "
    "232, 250, 218, 196, 6, 254, 139, 0, 172, 18, 189]";  // NOLINT

const uint8_t MOCK_SERVER_REPLY[] = {
    2,   0,   0,   0,   0,   0,   0,   0,   240, 197, 94,  143, 166, 246, 59,
    57,  250, 7,   154, 51,  170, 222, 189, 5,   77,  90,  79,  68,  211, 130,
    31,  96,  219, 34,  219, 248, 80,  166, 2,   83,  242, 90,  143, 208, 103,
    221, 39,  28,  112, 182, 28,  254, 38,  112, 154, 184, 61,  212, 169, 213,
    46,  116, 27,  78,  146, 247, 222, 46,  93,  65,  241, 33,  184, 110, 224,
    77,  5,   103, 212, 99,  62,  103, 79,  69,  208, 99,  9,   219, 101, 236,
    53,  150, 108, 110, 38,  113, 88,  90,  147, 68,  201, 202, 5,   52,  240,
    197, 94,  143, 166, 246, 59,  57,  250, 7,   154, 51,  170, 222, 189, 5,
    77,  90,  79,  68,  211, 130, 31,  96,  219, 34,  219, 248, 80,  166, 2,
    83,  52,  189, 154, 72,  93,  113, 0,   160, 253, 187, 109, 239, 33,  9,
    132, 172, 184, 237, 10,  197, 253, 137, 79,  214, 87,  124, 115, 166, 102,
    10,  27,  72,  246, 242, 50,  135, 203, 224, 245, 216, 42,  85,  52,  249,
    172, 129, 67,  206, 63,  61,  202, 10,  35,  31,  106, 186, 174, 108, 249,
    52,  197, 63,  12,  67};

void TestEndToEnd() {
  std::string sig1 = "signal1";
  std::string sig2 = "signal2";
  const char* input[] = {sig1.c_str(), sig2.c_str()};

  int size_input = sizeof(input) / sizeof(input[0]);

  private_channel::ResultChallenge result =
      private_channel::start_challenge(input, size_input, SERVER_PK);

  assert(result.key_size == KEY_SIZE);
  assert(result.error == false);

  int size_enc_input = sizeof(MOCK_SERVER_REPLY) / sizeof(MOCK_SERVER_REPLY[0]);

  private_channel::free_first_round_result(result);
}

void TestBadPubkey() {
  std::string sig1 = "signal1";
  std::string sig2 = "signal2";
  const char* input[] = {sig1.c_str(), sig2.c_str()};

  int size_input = sizeof(input) / sizeof(input[0]);

  private_channel::ResultChallenge result =
      private_channel::start_challenge(input, size_input, SERVER_PK_MALFORMED);

  // error should be true
  assert(result.error == true);
}

int main() {
  TestEndToEnd();
  TestBadPubkey();
}

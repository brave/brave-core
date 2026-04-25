/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_compiled_instruction.h"

#include <vector>

#include "brave/components/brave_wallet/common/solana_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaCompiledInstructionUnitTest, SerializeDeserialize) {
  SolanaCompiledInstruction compiled_ins(1, {0, 1, 2}, {1, 2});

  std::vector<uint8_t> expected_bytes = {// program_id_index
                                         1,
                                         // account_indexes compact array
                                         3, 0, 1, 2,
                                         // data compact array
                                         2, 1, 2};

  std::vector<uint8_t> bytes;
  compiled_ins.Serialize(&bytes);
  EXPECT_EQ(bytes, expected_bytes);

  size_t bytes_index = 0;
  auto deserialized_compiled_ins =
      SolanaCompiledInstruction::Deserialize(bytes, &bytes_index);
  ASSERT_TRUE(deserialized_compiled_ins);
  EXPECT_EQ(deserialized_compiled_ins, compiled_ins);
  EXPECT_EQ(bytes_index, bytes.size());

  bytes.clear();
  deserialized_compiled_ins->Serialize(&bytes);
  EXPECT_EQ(bytes, expected_bytes);
}

}  // namespace brave_wallet

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_header.h"

#include "base/strings/strcat.h"  // IWYU pragma: export
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(PolkadotBlockHeader, BlockHashing) {
  {
    /*

    Genesis block for Westend

    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "number":"0x0",
        "stateRoot":"0x7e92439a94f79671f9cade9dff96a094519b9001a7432244d46ab644bb6f746f",
        "extrinsicsRoot":"0x03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314",
        "digest":{"logs":[]}}}

    */
    PolkadotBlockHeader header;
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x7e92439a94f79671f9cade9dff96a094519b9001a7432244d46ab644bb6f746f",
        header.state_root));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314",
        header.extrinsics_root));

    EXPECT_TRUE(base::HexStringToBytes(
        base::StrCat({base::HexEncodeLower(compact_scale_encode_u32(0))}),
        &header.encoded_logs));

    EXPECT_EQ(
        base::HexEncodeLower(header.GetHash()),
        "e143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e");
  }

  {
    /*
      Westend block.

      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0x3f25d04a0bc013690449554f2fdf93cd5db67be4a54282edc8bab8fc812c12f2",
          "number":"0xc7bc72",
          "stateRoot":"0x6d6e099ed4d7264dbe85e5b957768685d5301c8aa6446bba5b6aeaa46966accf",
          "extrinsicsRoot":"0x0dc76f4c9edd6695dd390b3a609c28aa65c41ee4da060a09530d7c8bb063fd41",
          "digest":{
            "logs":[
              "0x0642414245b501030b0000003d638f10000000008432a99fa96f6fbee0dabdb01d2201e8e61bc1b0b4852340ee9f179db66507364a916e56341350e2a426aae6075d4ea966c73ab522f6d5b3001137a2c71b600164ab70e127c531d798eadd44da7e63e4e77904f8d15f580d1123f495a8fc1700",
              "0x0542414245010152fd10b29fafa11a76c50119261534c8c820884c5bd917bd2ff00343ded0ef1b759dc0866a12d9d3fa1373662ee45ac3ed7d2ac585be57035b47b353c8cc8586"
            ]
          }
        }
      }
    */

    PolkadotBlockHeader header;
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x3f25d04a0bc013690449554f2fdf93cd5db67be4a54282edc8bab8fc812c12f2",
        header.parent_hash));

    EXPECT_TRUE(base::HexStringToUInt("0xc7bc72", &header.block_number));

    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x6d6e099ed4d7264dbe85e5b957768685d5301c8aa6446bba5b6aeaa46966accf",
        header.state_root));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x0dc76f4c9edd6695dd390b3a609c28aa65c41ee4da060a09530d7c8bb063fd41",
        header.extrinsics_root));

    EXPECT_TRUE(base::HexStringToBytes(
        base::StrCat(
            {base::HexEncodeLower(compact_scale_encode_u32(2)),
             R"(0642414245b501030b0000003d638f10000000008432a99fa96f6fbee0dabdb01d2201e8e61bc1b0b4852340ee9f179db66507364a916e56341350e2a426aae6075d4ea966c73ab522f6d5b3001137a2c71b600164ab70e127c531d798eadd44da7e63e4e77904f8d15f580d1123f495a8fc1700)",
             R"(0542414245010152fd10b29fafa11a76c50119261534c8c820884c5bd917bd2ff00343ded0ef1b759dc0866a12d9d3fa1373662ee45ac3ed7d2ac585be57035b47b353c8cc8586)"}),
        &header.encoded_logs));

    EXPECT_EQ(
        base::HexEncodeLower(header.GetHash()),
        "89f3078e21bacfecb8f892b43f995695f2ae28bc198d67e9e775756a524945be");
  }

  {
    /*
      Westend block.

      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0xd0bab120ed5de3bbd2cddfd7839a5eb045171988f53a6a51409b47ae96092e9a",
          "number":"0x1c45a9f",
          "stateRoot":"0x320b04be81a87d5c6ceed6c40b933135373cdee52c7abab66ebadbd254fc3c49",
          "extrinsicsRoot":"0xcddf78abdf0cf9eafd030accb10fd09255a23e7a27a79a715976efc87f56b860",
          "digest":{
            "logs":[
              "0x0642414245b50103110000009089951100000000484aca2d37ff7c027f36cfa05c61e96d727cf8f05d607718aac471fc4adf4c2b3ee9fa490164f3940f0f8811ededfb3ca41cee8352de675739c02244ab4a5400f20ba9f17802860380d545f6a31b8f473568871616542843af74a4f50d417901",
              "0x04424545468403fec302a6aba3ca535e4a1728789828bce6e662984b02dab84b73877c329abeae",
              "0x05424142450101ee5f00146ee70997c7e4c4043a82c39fa75fb377182d71f5101382e1cf68f144ca73c2383f315fbe95aeaa5c7b9130164b1141dce7c234b81eb8c2d719259d8d"
            ]
          }
        }
      }
    */

    PolkadotBlockHeader header;
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0xd0bab120ed5de3bbd2cddfd7839a5eb045171988f53a6a51409b47ae96092e9a",
        header.parent_hash));

    EXPECT_TRUE(base::HexStringToUInt("0x1c45a9f", &header.block_number));

    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x320b04be81a87d5c6ceed6c40b933135373cdee52c7abab66ebadbd254fc3c49",
        header.state_root));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0xcddf78abdf0cf9eafd030accb10fd09255a23e7a27a79a715976efc87f56b860",
        header.extrinsics_root));

    EXPECT_TRUE(base::HexStringToBytes(
        base::StrCat(
            {base::HexEncodeLower(compact_scale_encode_u32(3)),
             R"(0642414245b50103110000009089951100000000484aca2d37ff7c027f36cfa05c61e96d727cf8f05d607718aac471fc4adf4c2b3ee9fa490164f3940f0f8811ededfb3ca41cee8352de675739c02244ab4a5400f20ba9f17802860380d545f6a31b8f473568871616542843af74a4f50d417901)"
             R"(04424545468403fec302a6aba3ca535e4a1728789828bce6e662984b02dab84b73877c329abeae)"
             R"(05424142450101ee5f00146ee70997c7e4c4043a82c39fa75fb377182d71f5101382e1cf68f144ca73c2383f315fbe95aeaa5c7b9130164b1141dce7c234b81eb8c2d719259d8d)"}),
        &header.encoded_logs));

    EXPECT_EQ(
        base::HexEncodeLower(header.GetHash()),
        "524660d2802956c072ce112b5220ea6137cab900a87a806caf7d46ac0f225e42");
  }

  {
    /*
     Block from the Polkadot main relay chain.

    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "parentHash":"0x79c7d10ea92a424b84eaf6832d24be8ad38edca873178df8d6a2b2e3fcdd0aca",
        "number":"0x1c6b21d",
        "stateRoot":"0x68b4ff8641f943cd3c8632d91dafdeb8115b20046ef709bbc63d85e80c9fb213",
        "extrinsicsRoot":"0x3cb6c9334c203622604044dc2b155017853cfdaddd47f9310cab7e215878e251",
        "digest":{
          "logs":[
            "0x0642414245b501030801000062b49511000000001c776d503c6ce4fb778bf254bcbd3303983c8f8c4064b25835e632e327d2f57ac5c357846630e5a8f59f54610ba9e7bc7fca66f61dc9555ce38acc3ac9074c0597df44a2b3e06420b9d4392395f00a909135e16d41ec48761555ef27206fb603",
            "0x04424545468403d951e74bb9e6060d8983a20e36fb21e98b81a667ea6f679415af26f2ae789fa6",
            "0x0542414245010116e669292188cdd97f665e5e51cd208fed42e79e26688e07dba017971b33b476605322ee32c03141371ecd47b3ad9aa3f294e5c211ee30155c8fc69b0113a683"
          ]
        }
      }
    }
    */

    PolkadotBlockHeader header;
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x79c7d10ea92a424b84eaf6832d24be8ad38edca873178df8d6a2b2e3fcdd0aca",
        header.parent_hash));

    EXPECT_TRUE(base::HexStringToUInt("0x1c6b21d", &header.block_number));

    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x68b4ff8641f943cd3c8632d91dafdeb8115b20046ef709bbc63d85e80c9fb213",
        header.state_root));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x3cb6c9334c203622604044dc2b155017853cfdaddd47f9310cab7e215878e251",
        header.extrinsics_root));

    EXPECT_TRUE(base::HexStringToBytes(
        base::StrCat(
            {base::HexEncodeLower(compact_scale_encode_u32(3)),
             R"(0642414245b501030801000062b49511000000001c776d503c6ce4fb778bf254bcbd3303983c8f8c4064b25835e632e327d2f57ac5c357846630e5a8f59f54610ba9e7bc7fca66f61dc9555ce38acc3ac9074c0597df44a2b3e06420b9d4392395f00a909135e16d41ec48761555ef27206fb603)",
             R"(04424545468403d951e74bb9e6060d8983a20e36fb21e98b81a667ea6f679415af26f2ae789fa6)",
             R"(0542414245010116e669292188cdd97f665e5e51cd208fed42e79e26688e07dba017971b33b476605322ee32c03141371ecd47b3ad9aa3f294e5c211ee30155c8fc69b0113a683)"}),
        &header.encoded_logs));

    EXPECT_EQ(
        base::HexEncodeLower(header.GetHash()),
        "b54e12ebd0843d187f19d39fb8e9472a217cfa38ad063210e912f1ec98b070f9");
  }

  {
    /*
      Genesis block of the Polkadot main relay chain.

      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
          "number":"0x0",
          "stateRoot":"0x29d0d972cd27cbc511e9589fcb7a4506d5eb6a9e8df205f00472e5ab354a4e17",
          "extrinsicsRoot":"0x03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314",
          "digest":{"logs":[]}
        }
      }
    */

    PolkadotBlockHeader header;
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x0000000000000000000000000000000000000000000000000000000000000000",
        header.parent_hash));

    EXPECT_TRUE(base::HexStringToUInt("0x0", &header.block_number));

    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x29d0d972cd27cbc511e9589fcb7a4506d5eb6a9e8df205f00472e5ab354a4e17",
        header.state_root));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314",
        header.extrinsics_root));

    EXPECT_TRUE(base::HexStringToBytes(
        base::StrCat({base::HexEncodeLower(compact_scale_encode_u32(0))}),
        &header.encoded_logs));

    EXPECT_EQ(
        base::HexEncodeLower(header.GetHash()),
        "91b171bb158e2d3848fa23a9f1c25182fb8e20313b2c1eb49219da7a70ce90c3");
  }
}

}  // namespace brave_wallet

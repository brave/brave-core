/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/ipld_utils.h"
#include <algorithm>


#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "gtest/gtest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "base/logging.h"

namespace {
    //Header of the CAR file (base32 - cidv1 - dag-cbor - (sha2-256 : 256 : F88BC853804CF294FE417E4FA83028689FCDB1B1592C5102E1474DBC200FAB8B))
    // version: 1, roots: bafyreihyrpefhacm6kkp4ql6j6udakdit7g3dmkzfriqfykhjw6cad5lrm, bafyreidj5idub6mapiupjwjsyyxhyhedxycv4vihfsicm2vt46o7morwlm
    const std::vector<uint8_t> kCarv1HeaderData = {
    0xA2, 0x65, 0x72, 0x6F, 0x6F, 0x74, 0x73, 0x82, 0xD8, 0x2A, 0x58, 0x25, 0x00, 0x01, 0x71, 0x12, 
    0x20, 0xF8, 0x8B, 0xC8, 0x53, 0x80, 0x4C, 0xF2, 0x94, 0xFE, 0x41, 0x7E, 0x4F, 0xA8, 0x30, 0x28, 
    0x68, 0x9F, 0xCD, 0xB1, 0xB1, 0x59, 0x2C, 0x51, 0x02, 0xE1, 0x47, 0x4D, 0xBC, 0x20, 0x0F, 0xAB, 
    0x8B, 0xD8, 0x2A, 0x58, 0x25, 0x00, 0x01, 0x71, 0x12, 0x20, 0x69, 0xEA, 0x07, 0x40, 0xF9, 0x80, 
    0x7A, 0x28, 0xF4, 0xD9, 0x32, 0xC6, 0x2E, 0x7C, 0x1C, 0x83, 0xBE, 0x05, 0x5E, 0x55, 0x07, 0x2C, 
    0x90, 0x26, 0x6A, 0xB3, 0xE7, 0x9D, 0xF6, 0x3A, 0x36, 0x5B, 0x67, 0x76, 0x65, 0x72, 0x73, 0x69, 
    0x6F, 0x6E, 0x01, 0x5B
    };
}

class IpldUtilsUnitTest : public testing::Test {
 public:
  IpldUtilsUnitTest() = default;
  ~IpldUtilsUnitTest() override = default;

};

TEST_F(IpldUtilsUnitTest, DecodeCarv1Header) {
    // Valid CAR_V1 header
    auto result = ipfs::ipld::decode_carv1_header(kCarv1HeaderData);
    ASSERT_EQ(result.error.length(), 0UL);
    ASSERT_EQ(result.error_code, 0);
    ASSERT_EQ(result.data.version, 1UL);
    ASSERT_EQ(result.data.roots.size(), 2UL);
    ASSERT_TRUE(base::Contains(result.data.roots, "bafyreihyrpefhacm6kkp4ql6j6udakdit7g3dmkzfriqfykhjw6cad5lrm"));
    ASSERT_TRUE(base::Contains(result.data.roots, "bafyreidj5idub6mapiupjwjsyyxhyhedxycv4vihfsicm2vt46o7morwlm"));

    // Valid CAR_V1 header, wrong version. Changed version to 2 (offset 98)
    std::vector<uint8_t> carv1_header_data(kCarv1HeaderData); carv1_header_data[98] = 0x02;
    result = ipfs::ipld::decode_carv1_header(carv1_header_data);
    ASSERT_GT(result.error.length(), 0UL);
    ASSERT_EQ(result.error_code, 30);
    ASSERT_EQ(result.data.version, 2UL);
    ASSERT_EQ(result.data.roots.size(), 0UL);


    if(result.error.length() > 0) {
        LOG(INFO) << "Error: " << std::string(result.error.data(), result.error.length());
    } else {
        LOG(INFO) << "Header version:" << result.data.version << " Roots: ";
        for(const auto& item : result.data.roots) {
            LOG(INFO) << std::string(item.data(), item.length());
        }
    }

}
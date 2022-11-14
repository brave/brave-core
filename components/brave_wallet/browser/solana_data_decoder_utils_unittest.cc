/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_data_decoder_utils.h"

#include "base/base64.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SolanaDataDecoderUtilsTest : public testing::Test {
 public:
  SolanaDataDecoderUtilsTest() = default;
  ~SolanaDataDecoderUtilsTest() override = default;
};

TEST_F(SolanaDataDecoderUtilsTest, DecodeMetadataUri) {
  auto uri_encoded = base::Base64Decode(
      "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/"
      "R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAA"
      "AAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAMgAAABodHRwczovL2JhZmtyZWlmNHd4NTR3anI3c"
      "GdmdWczd2xhdHIzbmZudHNmd25ndjZldXNlYmJxdWV6cnhlbmo2Y2s0LmlwZnMuZHdlYi5sa"
      "W5rP2V4dD0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+"
      "pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/"
      "z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/"
      "UEDizyp6mLT1tUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==");
  ASSERT_TRUE(uri_encoded);
  auto uri = DecodeMetadataUri(*uri_encoded);
  ASSERT_TRUE(uri);
  EXPECT_EQ((*uri).spec(),
            "https://"
            "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
            "dweb.link/?ext=");
  ASSERT_FALSE(DecodeMetadataUri({}));
}

}  // namespace brave_wallet

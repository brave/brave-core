/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/security_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using std::placeholders::_1;

namespace confirmations {

class ConfirmationsSecurityHelperTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  ConfirmationsSecurityHelperTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsSecurityHelperTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    auto callback = std::bind(
        &ConfirmationsSecurityHelperTest::OnInitialize, this, _1);
    confirmations_->Initialize(callback);
  }

  void OnInitialize(const bool success) {
    EXPECT_EQ(true, success);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(ConfirmationsSecurityHelperTest, Sign) {
  // Arrange
  const std::map<std::string, std::string> headers = {
    {"digest", "SHA-256=qj7EBzMRSsGh4Rfu8Zha6MvPB2WftfJNeF8gt7hE9AY="}
  };

  const std::string key_id = "primary";

  const std::vector<uint8_t> private_key = {
      0xe9, 0xb1, 0xab, 0x4f, 0x44, 0xd3, 0x9e, 0xb0, 0x43, 0x23, 0x41, 0x1e,
      0xed, 0x0b, 0x5a, 0x2c, 0xee, 0xdf, 0xf0, 0x12, 0x64, 0x47, 0x4f, 0x86,
      0xe2, 0x9c, 0x70, 0x7a, 0x56, 0x61, 0x56, 0x50, 0x33, 0xce, 0xa0, 0x08,
      0x5c, 0xfd, 0x55, 0x1f, 0xaa, 0x17, 0x0c, 0x1d, 0xd7, 0xf6, 0xda, 0xaa,
      0x90, 0x3c, 0xdd, 0x31, 0x38, 0xd6, 0x1e, 0xd5, 0xab, 0x28, 0x45, 0xe2,
      0x24, 0xd5, 0x81, 0x44
  };

  // Act
  const std::string signature =
      helper::Security::Sign(headers, key_id, private_key);

  // Assert
  const std::string expected_signature = R"(keyId="primary",algorithm="ed25519",headers="digest",signature="m5CxS9uqI7DbZ5UDo51bcLRP2awqcUSU8tfc4t/ysrH47B8OJUG1roQyi6/pjSZj9VJuj296v77c/lxBlCn2DA==")";  // NOLINT
  EXPECT_EQ(expected_signature, signature);
}

TEST_F(ConfirmationsSecurityHelperTest, SignWithInvalidheaders) {
  // Arrange
  const std::map<std::string, std::string> headers = {};

  const std::string key_id = "primary";

  const std::vector<uint8_t> private_key = {
      0xe9, 0xb1, 0xab, 0x4f, 0x44, 0xd3, 0x9e, 0xb0, 0x43, 0x23, 0x41, 0x1e,
      0xed, 0x0b, 0x5a, 0x2c, 0xee, 0xdf, 0xf0, 0x12, 0x64, 0x47, 0x4f, 0x86,
      0xe2, 0x9c, 0x70, 0x7a, 0x56, 0x61, 0x56, 0x50, 0x33, 0xce, 0xa0, 0x08,
      0x5c, 0xfd, 0x55, 0x1f, 0xaa, 0x17, 0x0c, 0x1d, 0xd7, 0xf6, 0xda, 0xaa,
      0x90, 0x3c, 0xdd, 0x31, 0x38, 0xd6, 0x1e, 0xd5, 0xab, 0x28, 0x45, 0xe2,
      0x24, 0xd5, 0x81, 0x44
  };

  // Act
  const std::string signature =
      helper::Security::Sign(headers, key_id, private_key);

  // Assert
  const std::string expected_signature = "";
  EXPECT_EQ(expected_signature, signature);
}

TEST_F(ConfirmationsSecurityHelperTest, SignWithInvalidKeyId) {
  // Arrange
  const std::map<std::string, std::string> headers = {
    {"digest", "SHA-256=qj7EBzMRSsGh4Rfu8Zha6MvPB2WftfJNeF8gt7hE9AY="}
  };

  const std::string key_id = "";

  const std::vector<uint8_t> private_key = {
      0xe9, 0xb1, 0xab, 0x4f, 0x44, 0xd3, 0x9e, 0xb0, 0x43, 0x23, 0x41, 0x1e,
      0xed, 0x0b, 0x5a, 0x2c, 0xee, 0xdf, 0xf0, 0x12, 0x64, 0x47, 0x4f, 0x86,
      0xe2, 0x9c, 0x70, 0x7a, 0x56, 0x61, 0x56, 0x50, 0x33, 0xce, 0xa0, 0x08,
      0x5c, 0xfd, 0x55, 0x1f, 0xaa, 0x17, 0x0c, 0x1d, 0xd7, 0xf6, 0xda, 0xaa,
      0x90, 0x3c, 0xdd, 0x31, 0x38, 0xd6, 0x1e, 0xd5, 0xab, 0x28, 0x45, 0xe2,
      0x24, 0xd5, 0x81, 0x44
  };

  // Act
  const std::string signature =
      helper::Security::Sign(headers, key_id, private_key);

  // Assert
  const std::string expected_signature = "";
  EXPECT_EQ(expected_signature, signature);
}

TEST_F(ConfirmationsSecurityHelperTest, SignWithInvalidPrivateKey) {
  // Arrange
  const std::map<std::string, std::string> headers = {
    {"digest", "SHA-256=qj7EBzMRSsGh4Rfu8Zha6MvPB2WftfJNeF8gt7hE9AY="}
  };

  const std::string key_id = "primary";

  const std::vector<uint8_t> private_key = {};

  // Act
  const std::string signature =
      helper::Security::Sign(headers, key_id, private_key);

  // Assert
  const std::string expected_signature = "";
  EXPECT_EQ(expected_signature, signature);
}

TEST_F(ConfirmationsSecurityHelperTest, GenerateTokens) {
  // Arrange

  // Act
  const std::vector<Token> tokens = helper::Security::GenerateTokens(5);

  // Assert
  const size_t count = tokens.size();
  EXPECT_EQ(5UL, count);
}

TEST_F(ConfirmationsSecurityHelperTest, BlindTokens) {
  // Arrange
  const std::vector<Token> tokens = helper::Security::GenerateTokens(7);

  // Act
  const std::vector<BlindedToken> blinded_tokens =
      helper::Security::BlindTokens(tokens);

  // Assert
  EXPECT_EQ(tokens.size(), blinded_tokens.size());
}

TEST_F(ConfirmationsSecurityHelperTest, GetSHA256) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  // Act
  const std::vector<uint8_t> sha256 = helper::Security::GetSHA256(body);
  const std::string sha256_base64 = helper::Security::GetBase64(sha256);

  // Assert
  const std::string expected_sha256_base64 =
      "qj7EBzMRSsGh4Rfu8Zha6MvPB2WftfJNeF8gt7hE9AY=";
  EXPECT_EQ(expected_sha256_base64, sha256_base64);
}

TEST_F(ConfirmationsSecurityHelperTest, GetSHA256WithEmptyString) {
  // Arrange
  const std::string body = "";

  // Act
  const std::vector<uint8_t> sha256 = helper::Security::GetSHA256(body);

  // Assert
  EXPECT_TRUE(sha256.empty());
}

}  // namespace confirmations

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/request_signed_tokens_request.h"

#include <memory>
#include <string>
#include <vector>

#include "bat/confirmations/internal/security_helper.h"
#include "bat/confirmations/wallet_info.h"
#include "wrapper.hpp"  // NOLINT

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

using challenge_bypass_ristretto::Token;

class ConfirmationsRequestSignedTokensRequestTest : public ::testing::Test {
 protected:
  std::unique_ptr<RequestSignedTokensRequest> request_;

  ConfirmationsRequestSignedTokensRequestTest() :
      request_(std::make_unique<RequestSignedTokensRequest>()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsRequestSignedTokensRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  std::vector<Token> GetTokens(
      const int count) {
    std::vector<std::string> tokens_base64 = {
      "B2CbFJJ1gKJy9qs8NMburYj12VAqnVfFrQ2K2u0QwcBi1YoMMHQfRQeDbOQ62Z+WrCOTYLbZrBY7+j9hz2jLFL74KSQig7/PDbqIpmNYs6PpNUK3MpVc4dm5R9lkySQF",  // NOLINT
      "MHbZ2XgFtno4g7yq/tmFCr1sFuFrkE7D6JjVAmM70ZJrwH/EqYNaWL1qANSKXX9ghyiN8KUDThEhDTqhuBQ4v7gzNY2qHav9uiAmjqvLzDp7oxmUBFohmdkVlvWhxV0F",  // NOLINT
      "6WWlDOIHNs6Az23V+VM3QTDFFDkR9D0CGZSd27/cjo3eO5EDEzi9Ev5omoJwZQHqiObVgUXmRFRa8UYXsL4O4MvBsYlgGz9VyoBLXo0ethmEBowsrMubj3GR4CQaN6gB",  // NOLINT
      "IzhzMBc/rI8uzGuARaudvUYY662c0tqzYDPOfvbWRiThTTyH9fU13nmAmhkdtpoUnDlGTE37fLDpjWPlGdAd9r2qh++09+sa9xHV+V9SXHbr9gtJBybZMWr8vjQuslMM",  // NOLINT
      "eZDj3OGto3E0Uz0djk6Ilfgz+Ar4kMAXOL68iLTNycBPgoNnM1rtjaL4OqvSc1ascZhGCf6Js42B/wPVzUYuKMloATKmYs7Ym+ndXnuX0FV9XJs94tlIGcp4k0uOMcgB",  // NOLINT
      "8QNMIJuJfu9W4KURg1Y2coXyKjbJOQLmo6RIGg+tKkUcY7srgUpac8XteSwWy6o6YLDoNKXS21FmbZ4VHb+Bv2NVhBWooK0b8lwQAdVUax5+Ej77qK//GeyRmAcAQV8G",  // NOLINT
      "6ILvEIM3+kgacI6JFa5415qAdzcg6hccQzEyhMsqYFa3MZKzvcLEF57pFFRoaYw7nFDQL8v8CDG2iSUoBIk8bmeoUwgdXsgofHvSahcBSWawmcnn8ESJTkZPGgxaFgcA",  // NOLINT
      "VHDbhwcInhhjL/HhSF+NyYak7Zy24xzDDTpI+3rsEZ7iL4SYUdcVkFmJ+bg8QlmPv8UMTchPBP7CVtCc96jj5PwGMsvAB8t2TffdSK9SHBRx/ZINmYSb7x+GTTdqWugB",  // NOLINT
      "YbH2x8oMkQrPR0uX6h8LrcgXSrPlSg60FFfp8V+GM8eiCQTwPJ643kilmlKU/qNZM3e28Hw3W4GPAELnm/YxFzG6qJ4B1wVTBdl/myIa0M3QIdoOn2//+JH2u4jRtIgN",  // NOLINT
      "0/KAtyvRoYLhsQnwu4McuG7pglpDpi2BXQi//FwGu8m/O+iTh1Lijzpt2RCnotGh0Wid9efnojrYQH5NJv9GYOhUDX7yYHVjUorc6y6SkUaO1aATc42RciRQ0cmuQFQC"   // NOLINT
    };

    // Blinded tokens for above tokens:
    // "blindedTokens": [
    //   "iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=",
    //   "eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=",
    //   "1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8=",
    //   "9gtgRG1Fr6eQAfvIO7qGes2d0Zwnd7EXdOQI9ik0PRE=",
    //   "iGH6L3EtdYLQiD63D/elY3nfI2R8BJzq/ufPtFkTAXg=",
    //   "5mtjGDYwCC54EyFrr/5XoG98Cag7ughIYYr6mp8jmEQ=",
    //   "8vU5KFc8AXn45rcqTGdM9MeUvG+z8RL9o27Lir4izBY=",
    //   "huXHzk2SgmJkMauedoRUr/p86+jh1vKIa93O9FP2PQk=",
    //   "cg9nMhSA7hVoBFbq5rEGVF7kgAoXqMmPApmxO99aGVU=",
    //   "sBJB0ez2qw929moV4PZgw+AVbj7mBj9Mtqy3r2D0kw4="
    // ]

    const int modulo = tokens_base64.size();

    std::vector<Token> tokens;
    for (int i = 0; i < count; i++) {
      auto token_base64 = tokens_base64.at(i % modulo);
      auto token = Token::decode_base64(token_base64);

      tokens.push_back(token);
    }

    return tokens;
  }
};

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    BuildUrl) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.private_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  // Act
  const std::string url = request_->BuildUrl(wallet_info);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/token/d4ed0af0-bfa9-464b-abd7-67b29d891b8b";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    GetMethod) {
  // Arrange

  // Act
  const URLRequestMethod method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::POST, method);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    BuildBody) {
  // Arrange
  const std::vector<Token> tokens = GetTokens(3);
  const std::vector<BlindedToken> blinded_tokens =
      helper::Security::BlindTokens(tokens);

  // Act
  const std::string body = request_->BuildBody(blinded_tokens);

  // Assert
  const std::string expected_body = R"({"blindedTokens":["iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=","eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=","1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8="]})";  // NOLINT
  EXPECT_EQ(expected_body, body);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    HeadersCount) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.private_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  // Act
  const std::vector<std::string> headers =
      request_->BuildHeaders(body, wallet_info);

  // Assert
  const size_t count = headers.size();
  EXPECT_EQ(3UL, count);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    HeadersCountForInvalidWallet) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  const WalletInfo wallet_info;

  // Act
  const std::vector<std::string> headers =
      request_->BuildHeaders(body, wallet_info);

  // Assert
  const size_t count = headers.size();
  EXPECT_EQ(3UL, count);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    BuildDigestHeaderValue) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  // Act
  const std::string header_value = request_->BuildDigestHeaderValue(body);

  // Assert
  const std::string expected_header_value =
      "SHA-256=qj7EBzMRSsGh4Rfu8Zha6MvPB2WftfJNeF8gt7hE9AY=";
  EXPECT_EQ(expected_header_value, header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    BuildDigestHeaderValueForEmptyBody) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  // Act
  const std::string header_value = request_->BuildDigestHeaderValue(body);

  // Assert
  const std::string expected_header_value =
      "SHA-256=qj7EBzMRSsGh4Rfu8Zha6MvPB2WftfJNeF8gt7hE9AY=";
  EXPECT_EQ(expected_header_value, header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    BuildSignatureHeaderValue) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.private_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  // Act
  const std::string header_value =
      request_->BuildSignatureHeaderValue(body, wallet_info);

  // Assert
  std::string expected_header_value = R"(keyId="primary",algorithm="ed25519",headers="digest",signature="m5CxS9uqI7DbZ5UDo51bcLRP2awqcUSU8tfc4t/ysrH47B8OJUG1roQyi6/pjSZj9VJuj296v77c/lxBlCn2DA==")";  // NOLINT
  EXPECT_EQ(expected_header_value, header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    BuildSignatureHeaderValueForInvalidWallet) {
  // Arrange
  const std::string body = R"({"blindedTokens":["iiafV6PGoG+Xz6QR+k1WaYllcA+w0a1jcDqhbpFbvWw=","8g7v9CDoZuOjnABr8SYUJmCIRHlwkFpFBB6rLfEJlz0=","chNIADY97/IiLfWrE/P5T3p3SQIPZAc4fKkB8/4byHE=","4nW47xQoQB4+uEz3i6/sbb+FDozpdiOTG53E+4RJ9kI=","KO9qa7ZuGosA2xjM2+t3rn7/7Oljga6Ak1fgixjtp2U=","tIBcIB2Xvmx0S+2jwcYrnzPvf20GTconlWDSiWHqR3g=","aHtan+UcZF0II/SRoYm7bK27VJWDabNKjXKSVaoPPTY=","6jggPJK8NL1AedlRpJSrCC3+reG2BMGqHOmIPtAsmwA=","7ClK9P723ff+dOZxOZ0jSonmI5AHqsQU2Cn8FVAHID4=","zkm+vIFM0ko74m+XhnZirCh7YUc9ucDtQTC+kwhWvzQ=","+uoLhdsMEg42PRYiLs0lrAiGcmsPWX2D6hxmrcLUgC8=","GNE2ISRb52HSPq0maJ9YXmbbkzUpo5dSNIM9I1eD+F4=","iBx49OAb3LWQzKko8ZeVVAkwdSKRbDHViqR6ciBICCw=","IBC208b0z56kzjG2Z/iTwriZfMp2cqoQgk4vyJAKJy8=","Vq4l6jx8vSCmvTVFMg3Wz04Xz/oomFq4QRt26vRhDWg=","5KIAJPFrSrVW92FJXP7WmHLc7d5a4lfTrXTRKC9rYQg=","/s/SELS2gTDt1Rt7XaJ54RaGLQUL85cLpKW2mBLU2HU=","HkJpt3NbymO56XbB2Tj4S4xyIKSjltFTjn1QdC1rLnM=","/CQIGwgHAX2kFmaJ+65YtAbO4eSfUvMojVxZLq/p/AE=","8N33oYwImtxf9rbrAQ1v8VlRD4iHDVR11yhYCKKKGFs=","6EjTK0lYDGwFPrtMyTjiYIPV4OK7beMBTV6qrgFCwDw=","5LzZynN+sxbIfQKc92V3dC82x4e99oxChk7fFNvJHmM=","uEW1D0SU8VU5UGPOnkrCv3I+NFNa1fNPSjDy4gjvIm0=","aIEvt2dBwTp1vuxNYjLaP25YdV3FjCG23NDxZG+MXxg=","DIhrKTcba0NNoEKQAsSb1t9R3KVrkwX8fpLlOOLcMkI=","vNaRbm7RPEkFvNNdLKaNhyd7gkM+kNt23G0N4sLnLhU=","4MXZ/1hM6+xVzyYWY14tjIxCaisfrTgAUD3LLJHSd14=","6hsMVd3VIjKUhHmHQRQRKr7duSiKzL36b/J+Mc4DPHM=","OCe1Vv0l86izNn1PHw+yLw5e37J/Ab3oVyTPgFlS4Wc=","hu5fi5YMxsWfmK3uTspjcjwguBDeiYMGuV+vIzC8jlg=","Vs+EZRjtF+xUC3sYUZsvpND8ugLPz6Yl0jCcv4HO2Co=","7Pxgek1VUU+93o6PWUdKgQW7IkDmLsotSEg8H7xj93U=","avRL8coOl6cWJxKlvY9mHfw1FWIF14JnhNdxW00fqAM=","Vvo4hscwrZgOIuwkgUaxzyrcGQbUS1vCWcNgjEkhfUg=","ChsgA1m1hmWFt3r6xQqNCZVqx/tMMzEdpy++uccB3Cs=","MImbGYf4TyE9WW/jx381Spk0B9boASAyehwz1om9Ong=","ksPN5jCF2uN8d1io+xXVJhJXZs/DpQsPsoCZl8L9EgA=","4AApGEJLMC3rgYgUABQp9nTXeikDmS29a2wkUOXIQXU=","JOcObac9kXq8eD0aIU5S5DKWiA/Ggf4tBC58KD2xtRs=","CBHMKoOwelZhfmupH1bH5Yo6BxDSkT8G2Jfk4xKsgyU=","Al/1AAI4W68MEk6+Ay0xIGjxzvlX6IdnPV9KgO1RU0c=","MtKvUJzIOOvOw8y+XzBbUrgyPxvE/DID2qvB3VsmVEs=","oIaCqLv0kIG9BDZz5u0xj0/ZQqZQMCn7gkgIHVioSFc=","8N1j1xiNm8dY90J9HQaeKyG861i2AN0w9nkF4cieZzw=","wDMa7tUhloYanmLOivcgHyjCLr/OMaKtWdqbhadEmRM=","bCquxc5v8J/P2pqay5fpzcLkTqSVvwdZrAbbIOF8Lhs=","ODPBJiCcOMv48YS9QIcD0dH4bsfD2zQVsWkwBef1ci4=","eA9Yt1HOkDNvDT6+kq0093d7WI/L78/Gj9nAlmSYwzE=","wqt3REJpnoxOCSdHcJEiOsdBWb5yQD5jaTahFz40Tkc=","tLdemf03DyE7OkTS8QCZS8OT0JflCVO1CmCbA8i2SXI="]})";  // NOLINT

  const WalletInfo wallet_info;

  // Act
  const std::string header_value =
      request_->BuildSignatureHeaderValue(body, wallet_info);

  // Assert
  const std::string expected_header_value = "";
  EXPECT_EQ(expected_header_value, header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    GetAcceptHeaderValue) {
  // Arrange

  // Act
  const std::string accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ("application/json", accept_header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest,
    GetContentType) {
  // Arrange

  // Act
  const std::string content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ("application/json", content_type);
}

}  // namespace confirmations

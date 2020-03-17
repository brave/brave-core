/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionUtilTest.*

namespace braveledger_promotion {

class PromotionUtilTest : public testing::Test {
 public:
  ledger::PromotionPtr GetPromotion() {
    auto promotion = ledger::Promotion::New();
    auto credentials = ledger::PromotionCreds::New();

    credentials->tokens = R"([
          "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"
        ])";

    credentials->blinded_creds = R"([
          "Gggq6QFD8GszbAO2Lsjms9QtaIUGWyfcAeeXmTN0Jw0=",
          "gLmphI+RsPU5yz+q2XYENT7/Uaff+XiycP2EVVBfigY=",
          "jlc4M10scQHkUGwOVHMgbwA8RYvX9AO0rmH4aMB3RF0=",
          "ZJO37nIin+EbTFljcBI3nlYnGtlrHuWK2qpL3T1Ncyk=",
          "KtBca7FBlQ4NViuUy5L6ATpnVUy+dDNqUEJA55jLznc=",
          "uO3p3VcWjme2yPyWv6oW3tJZkssQhbK4+v71I7ll72E=",
          "8GRAJi6QWLmHAObOstTNxwhyPpovIXMq/dQygYg1i1s=",
          "5sR8RMl3G4ccNTA3cAQi2MrRZw8oimtis0LpekYVaUc=",
          "AoSWLibiRwhJrDgSSloKxJmhuNpUV6ujYMHK89sNAWA=",
          "hMN0GZoIohkYZgctWUbUFWf8QVXZtjmWlIwliQtyjSs=",
          "pMfJ2H+AdeIXjhXCzNmAoVNdPETRPfpWcwrRU328MWk=",
          "QknZlZdJMzPqSdzklI/rXrJseg1lQwgDo7gYAH++m0U=",
          "Tn63o5SFWpPWkWf6U7Eo9cwDiO57mz8xkkqYU8cXmyY=",
          "DBYuiyLicXPdSBoSAvQ+NIZCpWcmKfln+VWGftSiACE=",
          "stq8pyNaIoHba5mUnxqnOT4hfrm8oHDSjUGnvwwlmFM=",
          "UqiXg4LVAIKswfKQ3R6QHKjLs4isaWPvFUM68pogYEs=",
          "Ih6uOcRgTilvlhIFd8EtbIsWC7rZGo9KTjJFlt7t9iA=",
          "FgxolUdYYtPiwskea6S62Eilbj3hFz3tRIN6UEsbVRw=",
          "cvpRU/QSuIpFslB3V92ih36mNvjx6/1/F0Veksj+yhA=",
          "NKIlnAJowWE/a/yeJsHHQDPy3I0qF2A5eTfshgOKHAQ="
        ])";

    credentials->signed_creds = R"([
          "whyLpcq84WBfWSvRevORFeyhfdqLQnINPMpbtt8kJUM=",
          "1qgtLfj8MJihUhYRl5rE0TJZcTEAIwjxVc4QxpGlzRA=",
          "WJ3VUVIFLP3s5l4+gmEg8CeSiZ/jcAyx5mnHwZ96L30=",
          "zL/vNT8LcvHXm3ckNEKBCwM5ApL16gAieFePvAZfKUQ=",
          "qkFCSzokORAJJwAJrTgpfYY9J8uIZjuAe6jax+q0Pmo=",
          "npfth11Vvm9tTO773xZ8SY1b0orUHVJG3380XKMGvSw=",
          "FgJvxc1NQAJRyFUXh/2gGch+hiDnfMc3EC36d5zy9mY=",
          "4sCN5isvdPu5a/eqG+otvivCg91ua2Fu3aJDxDWspHs=",
          "ZGDqbP6a7S+o1UL3P8dGZp55SueW/1GXwk3FpCL5txM=",
          "dLWseCdi7zR3hOAdml7c5HvIWOHyQ0BhhfpjpIBRghM=",
          "WDiJnPj8SfTRPCI2u6cAG8GMSiSF3aRk9bIRruoR2wo=",
          "grCk/Ktag4ACaChEB5tPixuZB6SHz14YnN25p0YDuTc=",
          "Hu3yQVKi/Y8e/0QfNZ9ZAXOEDEJTjEwoKcm2VbtfClA=",
          "RvOPReTvHlv1JzNbwoGBtX6GeKmp2M8qVgbutrxXZ2s=",
          "MrEtLGpzpElqppRoW+45+OXLlTbXWXRzurqQsGmYQmM=",
          "0s3fmZAS8adnuGD90HQeKYwdDMP1+97QHD7FOhugayw=",
          "hPh7nzr7odsV6VwHhKlwDIFKloGQTpbi23qllJCi8B8=",
          "GtRRTXAmPk1MNFOhzx6+cRSwZP0uFXeDcNxfj4jHv28=",
          "iKCMZF+7eHxr/3Aeh4rjIM/b0GU7x5e9ZkHO3GqiXl4=",
          "6KoAiaSu8fCBaywEayQYOQASELa9yqL245GVMbBlmWc="
        ])";

    credentials->public_key = "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=";
    credentials->batch_proof = "xdWq0jwSs2Z9lhfpEUR1nYX/f3Q4LUa9Y1kmhGMD1At/tqGTJ0ogFREiBwhCflUl2AoQmAUSsELbHrFtC/dgAQ==";  // NOLINT

    promotion->id = "36baa4c3-f92d-4121-b6d9-db44cb273a02";
    promotion->credentials = std::move(credentials);

    return promotion;
  }
};

TEST_F(PromotionUtilTest, VerifyPublicKey) {
  auto promotion = ledger::Promotion::New();
  auto credentials = ledger::PromotionCreds::New();

  // null pointer
  bool result = VerifyPublicKey(nullptr);
  EXPECT_EQ(result, false);

  // credentials are not set
  result = VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, false);

  // keys are not formatted correctly
  promotion->credentials = std::move(credentials);
  promotion->public_keys = "fdsfsdds";
  result = VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, false);

  // keys doesn't match
  promotion->public_keys = "[\"orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=\"]";
  promotion->credentials->public_key = "dfsdfsdf";
  result = VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, false);

  // keys match
  promotion->public_keys = "[\"orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=\"]";
  promotion->credentials->public_key =
      "orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=";
  result = VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, true);
}

TEST_F(PromotionUtilTest, UnBlindTokensWorksCorrectly) {
  std::vector<std::string> unblinded_encoded_tokens;
  std::string error;

  UnBlindTokens(GetPromotion(), &unblinded_encoded_tokens, &error);

  EXPECT_EQ(error, "");
  EXPECT_EQ(unblinded_encoded_tokens.size(), 20u);
}

TEST_F(PromotionUtilTest, UnBlindTokensCredsNotCorrect) {
  std::vector<std::string> unblinded_encoded_tokens;
  std::string error;

  auto promotion = GetPromotion();
  promotion->credentials->blinded_creds = promotion->credentials->signed_creds;

  UnBlindTokens(std::move(promotion), &unblinded_encoded_tokens, &error);

  EXPECT_EQ(error,
      "Unblinded tokens size does not match signed tokens sent in!");
  EXPECT_EQ(unblinded_encoded_tokens.size(), 0u);
}

// std::string ParseOSToString(ledger::OperatingSystem os)

TEST_F(PromotionUtilTest, ParseOSToStringForWindows) {
  // Act
  const std::string os_string =
      ParseOSToString(ledger::OperatingSystem::WINDOWS);

  // Assert
  EXPECT_EQ(os_string, "windows");
}

TEST_F(PromotionUtilTest, ParseOSToStringForMacOS) {
  // Act
  const std::string os_string =
      ParseOSToString(ledger::OperatingSystem::MACOS);

  // Assert
  EXPECT_EQ(os_string, "osx");
}

TEST_F(PromotionUtilTest, ParseOSToStringForLinux) {
  // Act
  const std::string os_string =
      ParseOSToString(ledger::OperatingSystem::LINUX);

  // Assert
  EXPECT_EQ(os_string, "linux");
}

TEST_F(PromotionUtilTest, ParseOSToStringForUndefined) {
  // Act
  const std::string os_string =
      ParseOSToString(ledger::OperatingSystem::UNDEFINED);

  // Assert
  EXPECT_EQ(os_string, "undefined");
}

// std::string ParseClientInfoToString(ledger::ClientInfoPtr info)

TEST_F(PromotionUtilTest, ParseClientInfoToStringForAndroid) {
  // Arrange
  auto client_info = ledger::ClientInfo::New();
  client_info->platform = ledger::Platform::ANDROID_R;

  // Act
  const std::string client_info_string =
      ParseClientInfoToString(std::move(client_info));

  // Assert
  EXPECT_EQ(client_info_string, "android");
}

TEST_F(PromotionUtilTest, ParseClientInfoToStringForIos) {
  // Arrange
  auto client_info = ledger::ClientInfo::New();
  client_info->platform = ledger::Platform::IOS;

  // Act
  const std::string client_info_string =
      ParseClientInfoToString(std::move(client_info));

  // Assert
  EXPECT_EQ(client_info_string, "ios");
}

TEST_F(PromotionUtilTest, ParseClientInfoToStringForDesktop) {
  // Arrange
  auto client_info = ledger::ClientInfo::New();
  client_info->platform = ledger::Platform::DESKTOP;
  client_info->os = ledger::OperatingSystem::WINDOWS;

  // Act
  const std::string client_info_string =
      ParseClientInfoToString(std::move(client_info));

  // Assert
  EXPECT_EQ(client_info_string, "windows");
}

TEST_F(PromotionUtilTest, ParseClientInfoToStringWithNullptr) {
  // Act
  const std::string client_info_string = ParseClientInfoToString(nullptr);

  // Assert
  EXPECT_EQ(client_info_string, "");
}

// ledger::PromotionType ConvertStringToPromotionType(const std::string& type)

TEST_F(PromotionUtilTest, ConvertStringToPromotionTypeForUGP) {
  // Act
  const ledger::PromotionType promotion_type =
      ConvertStringToPromotionType("ugp");

  // Assert
  EXPECT_EQ(promotion_type, ledger::PromotionType::UGP);
}

TEST_F(PromotionUtilTest, ConvertStringToPromotionTypeForADS) {
  // Act
  const ledger::PromotionType promotion_type =
      ConvertStringToPromotionType("ads");

  // Assert
  EXPECT_EQ(promotion_type, ledger::PromotionType::ADS);
}

// ledger::ReportType ConvertPromotionTypeToReportType(
//     const ledger::PromotionType type)

TEST_F(PromotionUtilTest, ConvertPromotionTypeToReportTypeForUGP) {
  // Act
  ledger::ReportType report_type =
      ConvertPromotionTypeToReportType(ledger::PromotionType::UGP);

  // Assert
  EXPECT_EQ(report_type, ledger::ReportType::GRANT_UGP);
}

TEST_F(PromotionUtilTest, ConvertPromotionTypeToReportTypeForADS) {
  // Act
  ledger::ReportType report_type =
      ConvertPromotionTypeToReportType(ledger::PromotionType::ADS);

  // Assert
  EXPECT_EQ(report_type, ledger::ReportType::GRANT_AD);
}

// bool ParseFetchResponse(
//     const std::string& response,
//     ledger::PromotionList* list)

TEST_F(PromotionUtilTest, ParseFetchResponseWithNullptrList) {
  std::string response;

  // Act
  bool success = ParseFetchResponse(response, nullptr);

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(PromotionUtilTest, ParseFetchResponseWithNoResponseContent) {
  std::string response;
  ledger::PromotionList list;

  // Act
  bool success = ParseFetchResponse(response, &list);

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(PromotionUtilTest, ParseFetchResponseWithInvalidResponseContent) {
  std::string response = "qwertyuyiuoip";
  ledger::PromotionList list;

  // Act
  bool success = ParseFetchResponse(response, &list);

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(PromotionUtilTest, ParseFetchResponseWithCompletePromotion) {
  const std::string response = R"({"promotions":[{"id":"1234","type":"ugp","version":1,"available":false,"approximateValue":"100.000000","legacyClaimed":false,"expiresAt":"2","publicKeys":["\"ABC\""],"suggestionsPerGrant":1}]})";  // NOLINT 
  ledger::PromotionList promotion_list;

  // Act
  bool success = ParseFetchResponse(response, &promotion_list);

  // Assert
  EXPECT_TRUE(success);

  EXPECT_EQ(promotion_list.size(), static_cast<uint64_t>(1));

  for (auto& promotion : promotion_list) {
    EXPECT_EQ(promotion->id, "1234");
  }
}

TEST_F(PromotionUtilTest, ParseFetchResponseWithCompletePromotions) {
  const std::string response = R"({"promotions":[{"id":"1234","type":"ugp","version":1,"available":false,"approximateValue":"100.000000","legacyClaimed":false,"expiresAt":"2","publicKeys":["\"ABC\""],"suggestionsPerGrant":1},{"id":"5678","type":"ugp","version":1,"available":false,"approximateValue":"100.000000","legacyClaimed":false,"expiresAt":"2","publicKeys":["\"ABC\""],"suggestionsPerGrant":1}]})";  // NOLINT 
  ledger::PromotionList promotion_list;

  // Act
  bool success = ParseFetchResponse(response, &promotion_list);

  // Assert
  EXPECT_TRUE(success);

  EXPECT_EQ(promotion_list.size(), static_cast<uint64_t>(2));

  bool found_promotion_a = false;
  bool found_promotion_b = false;

  for (auto& promotion : promotion_list) {
    if (promotion->id == "1234") {
      found_promotion_a = true;
    } else if (promotion->id == "5678") {
      found_promotion_b = true;
    }
  }
  EXPECT_TRUE(found_promotion_a);
  EXPECT_TRUE(found_promotion_b);
}

TEST_F(PromotionUtilTest,
    ParseFetchResponseWithCompleteAndIncompletePromotions) {
  // Arrange
  const std::string response = R"({"promotions":[{"id":"1234","type":"ugp","version":1,"available":false,"approximateValue":"100.000000","legacyClaimed":false,"expiresAt":"2","publicKeys":["\"ABC\""],"suggestionsPerGrant":1},{"id":"5678","type":"ugp","version":1,"available":false,"approximateValue":"100.000000","legacyClaimed":false,"expiresAt":"2","publicKeys":["\"ABC\""]}]})";  // NOLINT 
  // Nb. Promotion ID "5678" is missing "suggestionsPerGrant"
  ledger::PromotionList promotion_list;

  // Act
  bool success = ParseFetchResponse(response, &promotion_list);

  // Assert
  EXPECT_FALSE(success);

  EXPECT_EQ(promotion_list.size(), static_cast<uint64_t>(1));

  for (auto& promotion : promotion_list) {
    EXPECT_EQ(promotion->id, "1234");
  }
}

// std::string ParseClaimTokenResponse(
//     const std::string& response);

TEST_F(PromotionUtilTest, ParseClaimTokenResponseWithNoContent) {
  // Arrange
  const std::string response = "";

  // Act
  const std::string claim_token = ParseClaimTokenResponse(response);

  // Assert
  EXPECT_EQ(claim_token, "");
}

TEST_F(PromotionUtilTest, ParseClaimTokenResponseWithInvalidContent) {
  // Arrange
  const std::string response = R"({"bob":"1234"})";

  // Act
  const std::string claim_token = ParseClaimTokenResponse(response);

  // Assert
  EXPECT_EQ(claim_token, "");
}

TEST_F(PromotionUtilTest, ParseClaimTokenResponseWithValidContent) {
  // Arrange
  const std::string response = R"({"claimId":"1234"})";

  // Act
  const std::string claim_token = ParseClaimTokenResponse(response);

  // Assert
  EXPECT_EQ(claim_token, "1234");
}

// void ParseSignedTokensResponse(
    // const std::string& response,
    // base::Value* result);

TEST_F(PromotionUtilTest, ParseSignedTokensResponseWithNoContent) {
  // Arrange
  const std::string response = "";
  base::Value parsed_response(base::Value::Type::DICTIONARY);

  // Act
  ParseSignedTokensResponse(response, &parsed_response);

  // Assert
  EXPECT_EQ(parsed_response.DictSize(), static_cast<uint32_t>(0));
}

/* TODO(masparrow): fix crash!
TEST_F(PromotionUtilTest,
    ParseSignedTokensResponseWithValidContentButNoResponseObject) {
  // Arrange
  const std::string response = R"({"batchProof":"1234","publicKey":"ABCD","signedCreds":["ABC"]})";  // NOLINT

  // Act
  ParseSignedTokensResponse(response, nullptr);

  // Assert
  EXPECT_TRUE(true);  // TODO(masparrow): Not sure how to qualify this test?
}
*/
TEST_F(PromotionUtilTest, ParseSignedTokensResponseMissingBatchProof) {
  // Arrange
  const std::string response = R"({"publicKey":"ABCD","signedCreds":["ABC"]})";  // NOLINT
  base::Value parsed_response(base::Value::Type::DICTIONARY);

  // Act
  ParseSignedTokensResponse(response, &parsed_response);

  // Assert
  EXPECT_EQ(parsed_response.DictSize(), static_cast<uint32_t>(0));
}

TEST_F(PromotionUtilTest, ParseSignedTokensResponseMissingPublicKey) {
  // Arrange
  const std::string response = R"({"batchProof":"1234","signedCreds":["ABC"]})";  // NOLINT
  base::Value parsed_response(base::Value::Type::DICTIONARY);

  // Act
  ParseSignedTokensResponse(response, &parsed_response);

  // Assert
  EXPECT_EQ(parsed_response.DictSize(), static_cast<uint32_t>(0));
}

TEST_F(PromotionUtilTest, ParseSignedTokensResponseMissingSignedCreds) {
  // Arrange
  const std::string response = R"({"batchProof":"1234","publicKey":"ABCD"})";  // NOLINT
  base::Value parsed_response(base::Value::Type::DICTIONARY);

  // Act
  ParseSignedTokensResponse(response, &parsed_response);

  // Assert
  EXPECT_EQ(parsed_response.DictSize(), static_cast<uint32_t>(0));
}

TEST_F(PromotionUtilTest, ParseSignedTokensResponseEmptySignedCreds) {
  // Arrange
  const std::string response = R"({"batchProof":"1234","publicKey":"ABCD","signedCreds":[]})";  // NOLINT
  base::Value parsed_response(base::Value::Type::DICTIONARY);

  // Act
  ParseSignedTokensResponse(response, &parsed_response);

  // Assert
  EXPECT_EQ(parsed_response.DictSize(), static_cast<uint32_t>(3));
  base::Value* signed_creds_value = parsed_response.FindListKey("signed_creds");
  EXPECT_TRUE(signed_creds_value != nullptr);
  base::Value::ListStorage& signed_creds = signed_creds_value->GetList();
  EXPECT_EQ(signed_creds.size(), static_cast<uint64_t>(0));
}

TEST_F(PromotionUtilTest, ParseSignedTokensResponseWithValidContent) {
  // Arrange
  const std::string response = R"({"batchProof":"1234","publicKey":"ABCD","signedCreds":["ABC"]})";  // NOLINT
  base::Value parsed_response(base::Value::Type::DICTIONARY);

  // Act
  ParseSignedTokensResponse(response, &parsed_response);

  // Assert
  EXPECT_EQ(parsed_response.DictSize(), static_cast<uint32_t>(3));

  base::Value* signed_creds_value = parsed_response.FindListKey("signed_creds");
  EXPECT_TRUE(signed_creds_value != nullptr);
  base::Value::ListStorage& signed_creds = signed_creds_value->GetList();
  EXPECT_EQ(signed_creds.size(), static_cast<uint64_t>(1));
  base::Value& signed_cred_value = signed_creds.front();
  const std::string& signed_cred = signed_cred_value.GetString();
  EXPECT_EQ(signed_cred, "ABC");

  const std::string* public_key = parsed_response.FindStringKey("public_key");
  EXPECT_TRUE(public_key != nullptr);
  EXPECT_EQ(*public_key, "ABCD");

  const std::string* batch_proof = parsed_response.FindStringKey("batch_proof");
  EXPECT_TRUE(batch_proof != nullptr);
  EXPECT_EQ(*batch_proof, "1234");
}

// std::unique_ptr<base::ListValue> ParseStringToBaseList(
//    const std::string& string_list);

// bool VerifyPublicKey(const ledger::PromotionPtr promotion)

TEST_F(PromotionUtilTest, VerifyPublicKeyWithNullptr) {
  // Arrange
  auto promotion = ledger::Promotion::New();
  promotion->credentials = ledger::PromotionCreds::New();

  // Act
  bool result = braveledger_promotion::VerifyPublicKey(nullptr);

  // Assert
  EXPECT_EQ(result, false);
}

TEST_F(PromotionUtilTest, VerifyPublicKeyWithoutCredentials) {
  // Arrange
  auto promotion = ledger::Promotion::New();
  promotion->credentials = ledger::PromotionCreds::New();

  // Act
  bool result = braveledger_promotion::VerifyPublicKey(promotion->Clone());

  // Assert
  EXPECT_EQ(result, false);
}

TEST_F(PromotionUtilTest, VerifyPublicKeyWithInvalidKeys) {
  // Arrange
  auto promotion = ledger::Promotion::New();
  promotion->credentials = ledger::PromotionCreds::New();
  promotion->public_keys = "fdsfsdds";

  // Act
  bool result = braveledger_promotion::VerifyPublicKey(promotion->Clone());

  // Assert
  EXPECT_EQ(result, false);
}

TEST_F(PromotionUtilTest, VerifyPublicKeyWithMismatchedKeys) {
  // Arrange
  auto promotion = ledger::Promotion::New();
  promotion->credentials = ledger::PromotionCreds::New();
  promotion->public_keys = "[\"orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=\"]";
  promotion->credentials->public_key = "dfsdfsdf";

  // Act
  bool result = braveledger_promotion::VerifyPublicKey(promotion->Clone());

  // Assert
  EXPECT_EQ(result, false);
}

TEST_F(PromotionUtilTest, VerifyPublicKeyWithMatchingKeys) {
  // Arrange
  auto promotion = ledger::Promotion::New();
  promotion->credentials = ledger::PromotionCreds::New();
  promotion->public_keys = "[\"orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=\"]";
  promotion->credentials->public_key =
      "orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=";

  // Act
  bool result = braveledger_promotion::VerifyPublicKey(promotion->Clone());

  // Assert
  EXPECT_EQ(result, true);
}

}  // namespace braveledger_promotion

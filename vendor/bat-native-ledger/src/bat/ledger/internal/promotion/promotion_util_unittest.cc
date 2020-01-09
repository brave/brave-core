/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionUtilTest.*

namespace braveledger_promotion {

class PromotionUtilTest : public testing::Test {
};

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

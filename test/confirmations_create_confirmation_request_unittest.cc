/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "confirmations_client_mock.h"
#include "bat-native-confirmations/src/confirmations_impl.h"
#include "bat-native-confirmations/src/create_confirmation_request.h"
#include "bat-native-confirmations/src/security_helper.h"
#include "bat-native-confirmations/include/bat/confirmations/wallet_info.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsCreateConfirmationRequestTest : public ::testing::Test {
 protected:
  MockConfirmationsClient* mock_confirmations_client_;
  ConfirmationsImpl* confirmations_;

  CreateConfirmationRequest* request_;

  ConfirmationsCreateConfirmationRequestTest() :
      mock_confirmations_client_(new MockConfirmationsClient()),
      confirmations_(new ConfirmationsImpl(mock_confirmations_client_)),
      request_(new CreateConfirmationRequest()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsCreateConfirmationRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
    delete request_;

    delete confirmations_;
    delete mock_confirmations_client_;
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
};

TEST_F(ConfirmationsCreateConfirmationRequestTest, BuildUrl) {
  // Arrange
  std::string confirmation_id = "c7f8c42d-6768-4dd7-8dc6-612cbba3ec21";
  std::string credential = "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRnZuU1RNSjZkU2VpblBJZGMzUDJYUWx2ODRZMXdjbGp6V21rZmluVlhIcz1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNDY1ZTA4YWQtMDNiZS00MmVlLTkwMmEtZGM4ODY4OGFhMmNiXCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJsYW5kZWRcIn0iLCJzaWduYXR1cmUiOiJvZGwvcDNiaWhWTnZxa1N0YkU1Y1kvbk51YkcrdDZZZyt3WEgyNkVzRWdlWXdCelRjR3RVb2sxaWtCVngwNEhJV0lLNWowVDYxZ3BoQk1ZekhvY1FtUT09IiwidCI6IjNNYTNyNzBTMXNyOWNXdHRRdFQ5U3I4TnhwT2VxWnRFV0VQem9NOGduWXRybC9FSjVMRjJ2eVEySDF0SzRqMDJkeVQ4WEZ6MHdyTGh2MlJMMzVON1VBPT0ifQ==";  // NOLINT

  // Act
  auto url = request_->BuildUrl(confirmation_id, credential);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/c7f8c42d-6768-4dd7-8dc6-612cbba3ec21/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRnZuU1RNSjZkU2VpblBJZGMzUDJYUWx2ODRZMXdjbGp6V21rZmluVlhIcz1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNDY1ZTA4YWQtMDNiZS00MmVlLTkwMmEtZGM4ODY4OGFhMmNiXCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJsYW5kZWRcIn0iLCJzaWduYXR1cmUiOiJvZGwvcDNiaWhWTnZxa1N0YkU1Y1kvbk51YkcrdDZZZyt3WEgyNkVzRWdlWXdCelRjR3RVb2sxaWtCVngwNEhJV0lLNWowVDYxZ3BoQk1ZekhvY1FtUT09IiwidCI6IjNNYTNyNzBTMXNyOWNXdHRRdFQ5U3I4TnhwT2VxWnRFV0VQem9NOGduWXRybC9FSjVMRjJ2eVEySDF0SzRqMDJkeVQ4WEZ6MHdyTGh2MlJMMzVON1VBPT0ifQ==";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::POST, method);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, BuildBody) {
  // Arrange
  std::string creative_instance_id = "465e08ad-03be-42ee-902a-dc88688aa2cb";

  std::string blinded_token_base64 =
      "FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token);

  // Act
  auto body = request_->BuildBody(payload);

  // Assert
  std::string expected_body = R"({"blindedPaymentToken":"FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=","creativeInstanceId":"465e08ad-03be-42ee-902a-dc88688aa2cb","payload":{},"type":"landed"})";  // NOLINT
  EXPECT_EQ(expected_body, body);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, HeadersCount) {
  // Arrange

  // Act
  auto headers = request_->BuildHeaders();

  // Assert
  auto count = headers.size();
  EXPECT_EQ(1UL, count);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetAcceptHeaderValue) {
  // Arrange

  // Act
  auto accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ(accept_header_value, "application/json");
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetContentType) {
  // Arrange

  // Act
  auto content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ(content_type, "application/json");
}

TEST_F(ConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTO) {
  // Arrange
  std::string creative_instance_id = "465e08ad-03be-42ee-902a-dc88688aa2cb";

  std::string blinded_token_base64 =
      "FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  // Act
  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token);

  // Assert
  std::string expected_payload = R"({"blindedPaymentToken":"FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=","creativeInstanceId":"465e08ad-03be-42ee-902a-dc88688aa2cb","payload":{},"type":"landed"})";  // NOLINT
  EXPECT_EQ(expected_payload, payload);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, CreateCredential) {
  // Arrange
  std::string unblinded_token_base64 = "PUfdKQM4YOp/4o9IK33FHbedHp9nm0uHfSHdIqZw4dxBoo7lIb+aFYffv0dxEbwnADigaiOsliXbjFgtspB9ZYYD9GKXVCCVrss3M9QjSr3a449R+evShkcjRVxDxWoF";  // NOLINT
  auto unblinded_token = UnblindedToken::decode_base64(unblinded_token_base64);

  std::string creative_instance_id = "465e08ad-03be-42ee-902a-dc88688aa2cb";

  std::string blinded_token_base64 =
      "aCmqXz88SL4jUoRphNUZ+bpO9vfcoXL2jfknynMN4l0=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token);

  // Act
  auto credential = request_->CreateCredential(unblinded_token, payload);

  // Assert
  std::string expected_credential = "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiYUNtcVh6ODhTTDRqVW9ScGhOVVorYnBPOXZmY29YTDJqZmtueW5NTjRsMD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNDY1ZTA4YWQtMDNiZS00MmVlLTkwMmEtZGM4ODY4OGFhMmNiXCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJsYW5kZWRcIn0iLCJzaWduYXR1cmUiOiJHaGs4NmMwWXNZeXZLb3R6WE1ycVJxUHk3aXgyV1JNVXRyU0dka0p4R2tKaE9ua2ErTWN0SmxNUjczRXVONGJKaXY3TWcyaTg4YzVpbDJiY1J1ZUZkdz09IiwidCI6IlBVZmRLUU00WU9wLzRvOUlLMzNGSGJlZEhwOW5tMHVIZlNIZElxWnc0ZHhCb283bEliK2FGWWZmdjBkeEVid25BRGlnYWlPc2xpWGJqRmd0c3BCOVpRPT0ifQ==";  // NOLINT
  EXPECT_EQ(expected_credential, credential);
}

}  // namespace confirmations

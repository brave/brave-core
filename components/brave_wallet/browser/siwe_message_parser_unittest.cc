/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/browser/siwe_message_parser.h"
#include "brave/components/brave_wallet/common/sign_message_request.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace brave_wallet {

namespace {
constexpr char kExampleAddress[] = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2";
constexpr char kExampleUri[] = "https://example.com/login";
constexpr char kExampleNonce[] = "32891756";
constexpr char kExampleISO8601Time[] = "2021-09-30T16:25:24Z";
constexpr char kExampleRequestId[] = "8b270b8a-d74b-459b-9933-81cb234d7c5e";
constexpr char kExampleResourceURI1[] =
    "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq";
constexpr char kExampleResourceURI2[] =
    "https://example.com/my-web2-claim.json";
}  // namespace

using State = SIWEMessageParser::State;

class SIWEMessageParserTest : public testing::Test {
 public:
  SIWEMessageParserTest() = default;
  ~SIWEMessageParserTest() override = default;

  std::string starting_token() {
    return SIWEMessageParser::GetStartingTokenForTesting();
  }
  std::string uri_token() { return SIWEMessageParser::GetURITokenForTesting(); }
  std::string version_token() {
    return SIWEMessageParser::GetVersionTokenForTesting();
  }
  std::string chain_id_token() {
    return SIWEMessageParser::GetChainIdTokenForTesting();
  }
  std::string nonce_token() {
    return SIWEMessageParser::GetNonceTokenForTesting();
  }
  std::string issued_at_token() {
    return SIWEMessageParser::GetIssuedAtTokenForTesting();
  }
  std::string expiration_time_token() {
    return SIWEMessageParser::GetExpirationTimeTokenForTesting();
  }
  std::string not_before_token() {
    return SIWEMessageParser::GetNotBeforeTokenForTesting();
  }
  std::string request_id_token() {
    return SIWEMessageParser::GetRequestIdTokenForTesting();
  }
  std::string resources_token() {
    return SIWEMessageParser::GetResourcesTokenForTesting();
  }
  std::string resources_seperator() {
    return SIWEMessageParser::GetResourcesSeperatorForTesting();
  }

  std::string valid_start_prefix() {
    return base::StrCat({"example.com", starting_token(), "\n"});
  }
  std::string valid_address_prefix() {
    return base::StrCat({valid_start_prefix(), kExampleAddress, "\n"});
  }
  std::string valid_statement_prefix() {
    return base::StrCat({valid_address_prefix(), "\n\n"});
  }
  std::string valid_uri_prefix() {
    return base::StrCat(
        {valid_statement_prefix(), uri_token(), kExampleUri, "\n"});
  }
  std::string valid_version_prefix() {
    return base::StrCat({valid_uri_prefix(), version_token(), "1\n"});
  }
  std::string valid_chain_id_prefix() {
    return base::StrCat({valid_version_prefix(), chain_id_token(), "5\n"});
  }
  std::string valid_nonce_prefix() {
    return base::StrCat(
        {valid_chain_id_prefix(), nonce_token(), kExampleNonce, "\n"});
  }
  std::string valid_issued_at_prefix() {
    return base::StrCat(
        {valid_nonce_prefix(), issued_at_token(), kExampleISO8601Time});
  }

  std::string valid_start_suffix() {
    return base::StrCat({starting_token(), "\n", kExampleAddress, "\n\n\n",
                         valid_statement_suffix()});
  }
  std::string valid_statement_suffix() {
    return base::StrCat({uri_token(), kExampleUri, "\n", version_token(), "1\n",
                         chain_id_token(), "5\n", nonce_token(), kExampleNonce,
                         "\n", issued_at_token(), kExampleISO8601Time});
  }

 protected:
  SIWEMessageParser parser_;
};

TEST_F(SIWEMessageParserTest, Start) {
  for (const std::string& invalid_case : {
           std::string(""),
           std::string("\n"),
           std::string("wants you to sign in with your Ethereum account:\n"),
           starting_token(),
           base::StrCat({starting_token(), "\n"}),
           base::StrCat({"example.com", starting_token(), kExampleAddress}),
           std::string(
               "example.comwants you to sign in with your Ethereum account:\n"),
           base::StrCat({"://example.com", starting_token(), "\n"}),
           base::StrCat({"example.com:abc", starting_token(), "\n"}),
           base::StrCat({"example.com::3388", starting_token(), "\n"}),
       }) {
    SCOPED_TRACE(testing::Message() << "\"" << invalid_case << "\"");
    EXPECT_FALSE(parser_.Parse(invalid_case));
    EXPECT_EQ(parser_.state(), State::kStart);
  }
  const struct {
    std::string origin_str;
    std::string expected_origin_str;
  } valid_cases[] = {
      {"example.com", "https://example.com"},
      {"example.com:3388", "https://example.com:3388"},
      {"http://example.com:3388", "http://example.com:3388"},
      {"https://example.com", "https://example.com"},
  };
  for (const auto& valid_case : valid_cases) {
    const std::string message =
        base::StrCat({valid_case.origin_str, valid_start_suffix()});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    auto result = parser_.Parse(message);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->origin,
              url::Origin::Create(GURL(valid_case.expected_origin_str)));
    // rest of the fields
    EXPECT_EQ(result->address, kExampleAddress);
    EXPECT_FALSE(result->statement);
    EXPECT_EQ(result->uri, GURL(kExampleUri));
    EXPECT_EQ(result->version, 1u);
    EXPECT_EQ(result->chain_id, "5");
    EXPECT_EQ(result->nonce, kExampleNonce);
    EXPECT_EQ(result->issued_at, kExampleISO8601Time);
    EXPECT_FALSE(result->expiration_time);
    EXPECT_FALSE(result->not_before);
    EXPECT_FALSE(result->request_id);
    EXPECT_FALSE(result->resources);
  }
}

TEST_F(SIWEMessageParserTest, AddressError) {
  for (const std::string& invalid_case : {
           "", "\n",
           "C02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2\n",        // without 0x
           "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2abcd\n",  // extra 4 bytes
           "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C75\n",          // 2 bytes short
           "0xxC02aaA39b223FE8D0A0e5C4F27eAD9083C75\n",         // 0xx prefix
       }) {
    const std::string message =
        base::StrCat({valid_start_prefix(), invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
    EXPECT_EQ(parser_.state(), State::kAddress);
  }
}

TEST_F(SIWEMessageParserTest, StatementError) {
  for (const std::string& invalid_case : {
           "", "\n", "statement", "\nstatement", "\nstatement\n",
           "\n\x80statement\n\n",  // non-ASCII char
       }) {
    const std::string message =
        base::StrCat({valid_address_prefix(), invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
    EXPECT_EQ(parser_.state(), State::kStatement);
  }
  for (const std::string& valid_case : {
           "\n\n",
           "\nexample statement\n\n",
       }) {
    const std::string message = base::StrCat(
        {valid_address_prefix(), valid_case, valid_statement_suffix()});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    auto result = parser_.Parse(message);
    ASSERT_TRUE(result);
    if (valid_case == "\n\n") {
      EXPECT_FALSE(result->statement);
    } else {
      ASSERT_TRUE(result->statement);
      EXPECT_EQ(*(result->statement), "example statement");
    }
    // rest of the fields
    EXPECT_EQ(result->origin, url::Origin::Create(GURL("https://example.com")));
    EXPECT_EQ(result->address, kExampleAddress);
    EXPECT_EQ(result->uri, GURL(kExampleUri));
    EXPECT_EQ(result->version, 1u);
    EXPECT_EQ(result->chain_id, "5");
    EXPECT_EQ(result->nonce, kExampleNonce);
    EXPECT_EQ(result->issued_at, kExampleISO8601Time);
    EXPECT_FALSE(result->expiration_time);
    EXPECT_FALSE(result->not_before);
    EXPECT_FALSE(result->request_id);
    EXPECT_FALSE(result->resources);
  }
}

TEST_F(SIWEMessageParserTest, CommonErrors) {
  const struct {
    std::string prefix;
    std::string token;
    std::string value;
    State state;
  } state_infos[] = {
      {valid_statement_prefix(), uri_token(), kExampleUri, State::kURI},
      {valid_uri_prefix(), version_token(), "1", State::kVersion},
      {valid_version_prefix(), chain_id_token(), "5", State::kChainId},
      {valid_chain_id_prefix(), nonce_token(), kExampleNonce, State::kNonce},
      {valid_nonce_prefix(), issued_at_token(), kExampleISO8601Time,
       State::kIssuedAt},
      {valid_issued_at_prefix(), expiration_time_token(), kExampleISO8601Time,
       State::kOptionalFields},
      {valid_issued_at_prefix(), not_before_token(), kExampleISO8601Time,
       State::kOptionalFields},
      {valid_issued_at_prefix(), request_id_token(), kExampleRequestId,
       State::kOptionalFields},
      {valid_issued_at_prefix(), resources_token(), kExampleResourceURI1,
       State::kOptionalFields},
  };
  for (const auto& state_info : state_infos) {
    for (const auto& invalid_case : {
             std::string(""), std::string("\n"), state_info.token,
             state_info.value,
             base::StrCat({state_info.token, state_info.value}),
             base::StrCat({state_info.token, "\n"}),
             base::StrCat({state_info.value, "\n"}),
             base::StrCat({state_info.token, "\n", state_info.value, "\n"}),
             base::StrCat({"\n", state_info.token, state_info.value, "\n"}),
             base::StrCat(
                 {"\n", state_info.token, "\n", state_info.value, "\n"}),
             base::StrCat({"\n", state_info.token, "\n", state_info.value}),
             base::StrCat({"abc", state_info.token, state_info.value, "\n"}),
             base::StrCat(
                 {state_info.token, state_info.token, state_info.value, "\n"}),
             base::StrCat({state_info.token.substr(state_info.token.size() - 1),
                           state_info.value,
                           "\n"}),  // ex. URI:https://example.com/login
         }) {
      std::string message;
      if (state_info.state == State::kOptionalFields) {
        // This is valid for optional field
        if (invalid_case ==
            base::StrCat({state_info.token, state_info.value})) {
          continue;
        }
        message = base::StrCat({state_info.prefix, "\n", invalid_case});
        // No trailing newline is valid for IssuedAt
      } else if (state_info.state == State::kIssuedAt &&
                 invalid_case ==
                     base::StrCat({state_info.token, state_info.value})) {
        continue;
      } else {
        message = base::StrCat({state_info.prefix, invalid_case});
      }
      SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
      EXPECT_FALSE(parser_.Parse(message));
      EXPECT_EQ(parser_.state(), state_info.state);
    }
  }
}

TEST_F(SIWEMessageParserTest, URIError) {
  for (const auto& invalid_case : {
           base::StrCat({uri_token(), std::string(kExampleUri).substr(8),
                         "\n"}),  // URI: example.com/login
           base::StrCat({uri_token(), "example", "\n"}),
       }) {
    const std::string message =
        base::StrCat({valid_statement_prefix(), invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
    EXPECT_EQ(parser_.state(), State::kURI);
  }
}

TEST_F(SIWEMessageParserTest, VersionError) {
  for (const auto& invalid_case : {
           base::StrCat({version_token(), "abc123", "\n"}),
           base::StrCat({version_token(), "123", "\n"}),
       }) {
    const std::string message =
        base::StrCat({valid_uri_prefix(), invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
    EXPECT_EQ(parser_.state(), State::kVersion);
  }
}

TEST_F(SIWEMessageParserTest, NonceError) {
  for (const auto& invalid_case : {
           base::StrCat({nonce_token(), "3289", "\n"}),  // less than 8 chars
           base::StrCat(
               {nonce_token(), kExampleNonce, "(^.<)\n"}),  // non-alphanumeric
       }) {
    const std::string message =
        base::StrCat({valid_chain_id_prefix(), invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
    EXPECT_EQ(parser_.state(), State::kNonce);
  }
}

// ExpirationTime, NotBefore, RequestId
TEST_F(SIWEMessageParserTest, OptionalStringFields) {
  const struct {
    std::string token;
    std::string value;
  } state_infos[] = {
      {expiration_time_token(), kExampleISO8601Time},
      {not_before_token(), kExampleISO8601Time},
      {request_id_token(), kExampleRequestId},
  };
  for (const auto& state_info : state_infos) {
    const std::string message = base::StrCat(
        {valid_issued_at_prefix(), "\n", state_info.token, state_info.value});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    auto result = parser_.Parse(message);
    ASSERT_TRUE(result);
    absl::optional<std::string> field;
    if (state_info.token == expiration_time_token()) {
      field = result->expiration_time;
      EXPECT_FALSE(result->not_before);
      EXPECT_FALSE(result->request_id);
    } else if (state_info.token == not_before_token()) {
      field = result->not_before;
      EXPECT_FALSE(result->expiration_time);
      EXPECT_FALSE(result->request_id);
    } else {
      field = result->request_id;
      EXPECT_FALSE(result->expiration_time);
      EXPECT_FALSE(result->not_before);
    }
    ASSERT_TRUE(field);
    EXPECT_EQ(*field, state_info.value);
    EXPECT_EQ(parser_.state(), State::kEnd);
    // rest of the fields
    EXPECT_EQ(result->origin, url::Origin::Create(GURL("https://example.com")));
    EXPECT_EQ(result->address, kExampleAddress);
    EXPECT_FALSE(result->statement);
    EXPECT_EQ(result->uri, GURL(kExampleUri));
    EXPECT_EQ(result->version, 1u);
    EXPECT_EQ(result->chain_id, "5");
    EXPECT_EQ(result->nonce, kExampleNonce);
    EXPECT_EQ(result->issued_at, kExampleISO8601Time);
    EXPECT_FALSE(result->resources);
  }
}

TEST_F(SIWEMessageParserTest, RequestIdError) {
  // contains non-pchar
  const std::string message =
      base::StrCat({valid_issued_at_prefix(), kExampleRequestId, ">|||<\n"});
  EXPECT_FALSE(parser_.Parse(message));
  EXPECT_EQ(parser_.state(), State::kOptionalFields);
}

TEST_F(SIWEMessageParserTest, OptionalResources) {
  for (const auto& invalid_case : {
           base::StrCat(
               {resources_token(), "\n",
                resources_seperator().substr(resources_seperator().size() - 1),
                kExampleResourceURI1}),
           base::StrCat({resources_token(), "\n", "+ ", kExampleResourceURI1}),
           base::StrCat(
               {resources_token(), "\n", resources_seperator(), "abc"}),
       }) {
    const std::string message =
        base::StrCat({valid_issued_at_prefix(), "\n", invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
    EXPECT_EQ(parser_.state(), State::kOptionalFields);
  }
  const std::string message =
      base::StrCat({valid_issued_at_prefix(), "\n", resources_token(), "\n",
                    resources_seperator(), kExampleResourceURI1, "\n",
                    resources_seperator(), kExampleResourceURI2});
  auto result = parser_.Parse(message);
  ASSERT_TRUE(result->resources);
  EXPECT_EQ(result->resources->size(), 2u);
  EXPECT_EQ(result->resources->at(0), GURL(kExampleResourceURI1));
  EXPECT_EQ(result->resources->at(1), GURL(kExampleResourceURI2));
  // rest of the fields
  EXPECT_EQ(result->origin, url::Origin::Create(GURL("https://example.com")));
  EXPECT_EQ(result->address, kExampleAddress);
  EXPECT_FALSE(result->statement);
  EXPECT_EQ(result->uri, GURL(kExampleUri));
  EXPECT_EQ(result->version, 1u);
  EXPECT_EQ(result->chain_id, "5");
  EXPECT_EQ(result->nonce, kExampleNonce);
  EXPECT_EQ(result->issued_at, kExampleISO8601Time);
  EXPECT_FALSE(result->expiration_time);
  EXPECT_FALSE(result->not_before);
  EXPECT_FALSE(result->request_id);
}

TEST_F(SIWEMessageParserTest, OptionalFieldsOrder) {
  for (const auto& invalid_case : {
           base::StrCat({not_before_token(), kExampleISO8601Time, "\n",
                         request_id_token(), kExampleRequestId, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1, "\n", expiration_time_token(),
                         kExampleISO8601Time}),
           base::StrCat({not_before_token(), kExampleISO8601Time, "\n",
                         expiration_time_token(), kExampleISO8601Time}),
           base::StrCat({request_id_token(), kExampleRequestId, "\n",
                         expiration_time_token(), kExampleISO8601Time}),
           base::StrCat({resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1, "\n", expiration_time_token(),
                         kExampleISO8601Time}),
           base::StrCat({request_id_token(), kExampleRequestId, "\n",
                         not_before_token(), kExampleISO8601Time}),
           base::StrCat({resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1, "\n", not_before_token(),
                         kExampleISO8601Time}),
           base::StrCat({resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1, "\n", request_id_token(),
                         kExampleRequestId}),
       }) {
    const std::string message =
        base::StrCat({valid_issued_at_prefix(), "\n", invalid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_FALSE(parser_.Parse(message));
  }
  for (const auto& valid_case : {
           base::StrCat({expiration_time_token(), kExampleISO8601Time, "\n",
                         not_before_token(), kExampleISO8601Time, "\n",
                         request_id_token(), kExampleRequestId, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1}),
           base::StrCat({expiration_time_token(), kExampleISO8601Time, "\n",
                         request_id_token(), kExampleRequestId, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1}),
           base::StrCat({expiration_time_token(), kExampleISO8601Time, "\n",
                         not_before_token(), kExampleISO8601Time, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1}),
           base::StrCat({expiration_time_token(), kExampleISO8601Time, "\n",
                         not_before_token(), kExampleISO8601Time, "\n",
                         request_id_token(), kExampleRequestId}),
           base::StrCat({not_before_token(), kExampleISO8601Time, "\n",
                         request_id_token(), kExampleRequestId, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1}),
           base::StrCat({not_before_token(), kExampleISO8601Time, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1}),
           base::StrCat({not_before_token(), kExampleISO8601Time, "\n",
                         request_id_token(), kExampleRequestId}),
           base::StrCat({request_id_token(), kExampleRequestId, "\n",
                         resources_token(), "\n", resources_seperator(),
                         kExampleResourceURI1}),
       }) {
    const std::string message =
        base::StrCat({valid_issued_at_prefix(), "\n", valid_case});
    SCOPED_TRACE(testing::Message() << "\"" << message << "\"");
    EXPECT_TRUE(parser_.Parse(message));
  }
}

}  // namespace brave_wallet

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/siwe_message_parser.h"

#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/sign_message_request.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

using base::StringTokenizer;

namespace brave_wallet {

namespace {
constexpr size_t kMinNonceLength = 8;
constexpr char kStartingToken[] =
    " wants you to sign in with your Ethereum account:";
constexpr char kURIToken[] = "URI: ";
constexpr char kVersionToken[] = "Version: ";
constexpr char kChainIdToken[] = "Chain ID: ";
constexpr char kNonceToken[] = "Nonce: ";
constexpr char kIssuedAtToken[] = "Issued At: ";
constexpr char kExpirationTimeToken[] = "Expiration Time: ";
constexpr char kNotBeforeToken[] = "Not Before: ";
constexpr char kRequestIdToken[] = "Request ID: ";
constexpr char kResourcesToken[] = "Resources:";
constexpr char kResourcesSeperator[] = "- ";

bool ConsumeDelim(StringTokenizer* tokenizer) {
  CHECK(tokenizer);
  if (!tokenizer->GetNext() || !tokenizer->token_is_delim()) {
    return false;
  }
  return true;
}

bool CheckField(const std::string& input, const std::string& msg_token) {
  std::string::size_type n = input.rfind(msg_token);
  if (n == std::string::npos || n != 0) {
    return false;
  }
  return true;
}

bool FillStringField(StringTokenizer* tokenizer,
                     const std::string& input,
                     const std::string& msg_token,
                     bool consume_delim,
                     std::string& field) {
  CHECK(tokenizer);
  if (!CheckField(input, msg_token)) {
    return false;
  }
  field = input.substr(msg_token.length());
  if (field.empty()) {
    return false;
  }
  if (consume_delim && !ConsumeDelim(tokenizer)) {
    return false;
  }
  return true;
}

bool FillOptionalStringField(StringTokenizer* tokenizer,
                             const std::string& msg_token,
                             absl::optional<std::string>& field) {
  CHECK(tokenizer);
  std::string field_str;
  if (!FillStringField(tokenizer, tokenizer->token(), msg_token, false,
                       field_str)) {
    return false;
  }
  field = field_str;
  return true;
}

bool FillOptionalResourcesField(StringTokenizer* tokenizer,
                                absl::optional<std::vector<GURL>>& field) {
  CHECK(tokenizer);
  if (!CheckField(tokenizer->token(), kResourcesToken)) {
    return false;
  }
  std::vector<GURL> urls;
  while (tokenizer->GetNext()) {
    if (tokenizer->token_is_delim()) {
      continue;
    }
    if (!CheckField(tokenizer->token(), kResourcesSeperator)) {
      return false;
    }
    auto url_str = tokenizer->token().substr(strlen(kResourcesSeperator));
    GURL url(url_str);
    if (!url.is_valid()) {
      return false;
    }
    urls.push_back(url);
  }
  if (!urls.size()) {
    return false;
  }
  field = urls;
  return true;
}

// https://datatracker.ietf.org/doc/html/rfc3986/#section-2.3
bool IsUnreservedChar(char c) {
  if (base::IsAsciiAlphaNumeric(c)) {
    return true;
  }
  switch (c) {
    case '-':
    case '.':
    case '_':
    case '~':
      return true;
  }
  return false;
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-2.2
bool IsSubDelimChar(char c) {
  switch (c) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=':
      return true;
  }
  return false;
}

// https://datatracker.ietf.org/doc/html/rfc3986/#section-3.3
bool IsPChar(char c) {
  if (IsUnreservedChar(c)) {
    return true;
  }
  if (IsSubDelimChar(c)) {
    return true;
  }
  switch (c) {
    case ':':
    case '[':
    case ']':
    case '@':
      return true;
  }
  return false;
}

}  // namespace

mojom::SIWEMessagePtr SIWEMessageParser::Parse(const std::string& message) {
  state_ = State::kStart;
  StringTokenizer tokenizer(message, "\n");
  tokenizer.set_options(StringTokenizer::RETURN_DELIMS);
  auto result = mojom::SIWEMessage::New();
  bool seeking_optional_fields = false;
  while (tokenizer.GetNext() || seeking_optional_fields) {
    const std::string& token = tokenizer.token();
    switch (state_) {
      case State::kStart: {
        std::string::size_type n = token.find(kStartingToken);
        if (!n || n == std::string::npos) {
          return {};
        }
        // missing line feed
        if (token.substr(n) != kStartingToken) {
          return {};
        }
        const std::string& old_origin_str = token.substr(0, n);
        std::string new_origin_str(old_origin_str);
        // If scheme is not specified, https will be used by default.
        if (old_origin_str.find(url::kStandardSchemeSeparator) ==
            std::string::npos) {
          new_origin_str =
              base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                            old_origin_str});
        }
        GURL url(new_origin_str);
        if (!url.is_valid()) {
          return {};
        }
        result->origin = url::Origin::Create(url);
        if (!ConsumeDelim(&tokenizer)) {
          return {};
        }
        state_ = State::kAddress;
        break;
      }
      case State::kAddress:
        if (!EthAddress::IsValidAddress(token)) {
          return {};
        }
        result->address = token;
        if (!ConsumeDelim(&tokenizer)) {
          return {};
        }
        state_ = State::kStatement;
        break;
      case State::kStatement:
        // Check starting line feed and check it is not the end
        if (!tokenizer.token_is_delim() || !tokenizer.GetNext()) {
          return {};
        }
        // If it is already ending line, does nothing and move to next state
        if (!tokenizer.token_is_delim()) {
          if (!base::IsStringASCII(tokenizer.token())) {
            return {};
          }
          result->statement = tokenizer.token();
          // Consume both statement and ending line feed
          if (!ConsumeDelim(&tokenizer) || !ConsumeDelim(&tokenizer)) {
            return {};
          }
        }
        state_ = State::kURI;
        break;
      case State::kURI:
        if (!CheckField(token, kURIToken)) {
          return {};
        }
        result->uri = GURL(token.substr(strlen(kURIToken)));
        if (!result->uri.is_valid() || !ConsumeDelim(&tokenizer)) {
          return {};
        }
        state_ = State::kVersion;
        break;
      case State::kVersion:
        if (!CheckField(token, kVersionToken)) {
          return {};
        }
        if (!base::StringToUint(token.substr(strlen(kVersionToken)),
                                &result->version)) {
          return {};
        }
        // The only supported version should be 1
        if (result->version != 1 || !ConsumeDelim(&tokenizer)) {
          return {};
        }
        state_ = State::kChainId;
        break;
      case State::kChainId:
        if (!FillStringField(&tokenizer, token, kChainIdToken, true,
                             result->chain_id)) {
          return {};
        }
        state_ = State::kNonce;
        break;
      case State::kNonce:
        if (!FillStringField(&tokenizer, token, kNonceToken, true,
                             result->nonce)) {
          return {};
        }
        if (!base::ranges::all_of(result->nonce,
                                  &base::IsAsciiAlphaNumeric<char>)) {
          return {};
        }
        if (result->nonce.size() < kMinNonceLength) {
          return {};
        }
        state_ = State::kIssuedAt;
        break;
      case State::kIssuedAt:
        // IssuedAt is the last required field so we don't consume delimiter
        if (!FillStringField(&tokenizer, token, kIssuedAtToken, false,
                             result->issued_at)) {
          return {};
        }
        state_ = State::kOptionalFields;
        seeking_optional_fields = true;
        break;
      case State::kOptionalFields: {
        if (token.empty()) {
          state_ = State::kEnd;
          seeking_optional_fields = false;
          break;
        }
        // check and consume starting delimeter
        if (!tokenizer.token_is_delim() || !tokenizer.GetNext()) {
          return {};
        }
        bool has_expiration_time = FillOptionalStringField(
            &tokenizer, kExpirationTimeToken, result->expiration_time);
        // check order of first optional field
        if (has_expiration_time &&
            (result->not_before || result->request_id || result->resources)) {
          return {};
        }
        bool has_not_before = FillOptionalStringField(
            &tokenizer, kNotBeforeToken, result->not_before);
        // check order of second optional field
        if (has_not_before && (result->request_id || result->resources)) {
          return {};
        }
        bool has_request_id = FillOptionalStringField(
            &tokenizer, kRequestIdToken, result->request_id);
        if (result->request_id &&
            !base::ranges::all_of(*result->request_id, &IsPChar)) {
          return {};
        }
        // check order of third optional field
        if (has_request_id && result->resources) {
          return {};
        }
        // None of them matches
        if (!has_expiration_time && !has_not_before && !has_request_id &&
            !FillOptionalResourcesField(&tokenizer, result->resources)) {
          return {};
        }
        if (token.empty()) {
          state_ = State::kEnd;
          seeking_optional_fields = false;
        }
        break;
      }
      default:
        NOTREACHED();
    }
  }
  if (state_ != State::kEnd) {
    return {};
  } else {
    return result;
  }
}

// static
std::string SIWEMessageParser::GetStartingTokenForTesting() {
  return kStartingToken;
}

// static
std::string SIWEMessageParser::GetURITokenForTesting() {
  return kURIToken;
}

// static
std::string SIWEMessageParser::GetVersionTokenForTesting() {
  return kVersionToken;
}

// static
std::string SIWEMessageParser::GetChainIdTokenForTesting() {
  return kChainIdToken;
}

// static
std::string SIWEMessageParser::GetNonceTokenForTesting() {
  return kNonceToken;
}

// static
std::string SIWEMessageParser::GetIssuedAtTokenForTesting() {
  return kIssuedAtToken;
}
// static
std::string SIWEMessageParser::GetExpirationTimeTokenForTesting() {
  return kExpirationTimeToken;
}

// static
std::string SIWEMessageParser::GetNotBeforeTokenForTesting() {
  return kNotBeforeToken;
}

// static
std::string SIWEMessageParser::GetRequestIdTokenForTesting() {
  return kRequestIdToken;
}

// static
std::string SIWEMessageParser::GetResourcesTokenForTesting() {
  return kResourcesToken;
}

// static
std::string SIWEMessageParser::GetResourcesSeperatorForTesting() {
  return kResourcesSeperator;
}

}  // namespace brave_wallet

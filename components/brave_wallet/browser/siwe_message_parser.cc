/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/siwe_message_parser.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/strcat.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/eth_address.h"
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

std::optional<std::pair<std::string::size_type, std::string_view>> ExtractValue(
    std::string_view input,
    std::string_view field,
    bool expect_lf = true) {
  auto field_end = field.length();
  if (field_end == input.length()) {
    return std::nullopt;
  }
  auto field_pos = input.rfind(field);
  if (field_pos == std::string::npos || field_pos != 0) {
    return std::nullopt;
  }
  std::string::size_type n;
  if (expect_lf) {
    n = input.find('\n', field_end);
    if (n == field_end || n == std::string::npos) {
      return std::nullopt;
    }
  } else {
    n = input.length();
  }

  return std::make_pair(n, input.substr(field_end, n - field_end));
}

std::optional<std::pair<std::string::size_type, std::string_view>>
ExtractOptionalValue(std::string_view input,
                     std::string_view field,
                     bool is_resources = false) {
  auto effective_field = base::StrCat({"\n", field});
  auto field_end = effective_field.length();
  if (field_end == input.length()) {
    return std::nullopt;
  }
  auto field_pos = input.rfind(effective_field);
  if (field_pos == std::string::npos) {
    return std::make_pair(std::string::npos, input);
  }
  if (field_pos != 0) {
    return std::nullopt;
  }
  std::string::size_type n;
  if (is_resources) {
    n = input.length();
  } else {
    // Check if this is the last optional field
    n = input.find('\n', field_end);
    if (n == std::string::npos) {
      return std::make_pair(input.length(), input.substr(field_end));
    }
    if (n == field_end) {
      return std::nullopt;
    }
  }

  return std::make_pair(n, input.substr(field_end, n - field_end));
}

bool ShrinkMessage(std::string_view& msg_view, std::string_view::size_type n) {
  if (n > msg_view.size()) {
    return false;
  }
  msg_view.remove_prefix(n);
  return true;
}

}  // namespace

SIWEMessageParser::SIWEMessageParser() = default;
SIWEMessageParser::~SIWEMessageParser() = default;

mojom::SIWEMessagePtr SIWEMessageParser::Parse(const std::string& message) {
  state_ = State::kStart;
  auto result = mojom::SIWEMessage::New();
  std::string_view message_view(message);
  if (!ParseSchemeAndDomain(message_view, result->origin)) {
    return {};
  }
  if (!ParseAddress(message_view, result->address)) {
    return {};
  }
  if (!ParseStatement(message_view, result->statement)) {
    return {};
  }
  if (!ParseURI(message_view, result->uri)) {
    return {};
  }
  if (!ParseVersion(message_view, result->version)) {
    return {};
  }
  if (!ParseChainId(message_view, result->chain_id)) {
    return {};
  }
  if (!ParseNonce(message_view, result->nonce)) {
    return {};
  }
  if (!ParseIssuedAt(message_view, result->issued_at)) {
    return {};
  }
  if (state_ == State::kOptionalFields) {
    if (!ParseOptionalStringField(message_view, kExpirationTimeToken,
                                  result->expiration_time)) {
      return {};
    }
    if (!message_view.empty()) {
      if (!ParseOptionalStringField(message_view, kNotBeforeToken,
                                    result->not_before)) {
        return {};
      }
    }
    if (!message_view.empty()) {
      if (!ParseOptionalStringField(message_view, kRequestIdToken,
                                    result->request_id)) {
        return {};
      }
    }
    if (result->request_id &&
        !base::ranges::all_of(*result->request_id, &IsPChar)) {
      return {};
    }
    if (!message_view.empty()) {
      if (!ParseOptionalResources(message_view, result->resources)) {
        return {};
      }
    }
  }
  if (!message_view.empty()) {
    return {};
  }
  state_ = State::kEnd;
  return result;
}

bool SIWEMessageParser::ParseSchemeAndDomain(std::string_view& msg_view,
                                             url::Origin& origin) {
  std::string::size_type n =
      msg_view.find(base::StrCat({kStartingToken, "\n"}));
  if (!n || n == std::string::npos) {
    return false;
  }
  std::string_view old_origin_str(msg_view.begin(), msg_view.begin() + n);
  std::string new_origin_str(old_origin_str);
  // If scheme is not specified, https will be used by default.
  if (!base::Contains(old_origin_str, url::kStandardSchemeSeparator)) {
    new_origin_str = base::StrCat(
        {url::kHttpsScheme, url::kStandardSchemeSeparator, old_origin_str});
  }
  GURL url(new_origin_str);
  if (!url.is_valid()) {
    return false;
  }

  origin = url::Origin::Create(url);
  CHECK(ShrinkMessage(msg_view, n + strlen(kStartingToken) + 1));
  state_ = State::kAddress;
  return true;
}

bool SIWEMessageParser::ParseAddress(std::string_view& msg_view,
                                     std::string& address_out) {
  std::string::size_type n = msg_view.find('\n');
  if (!n || n == std::string::npos) {
    return false;
  }
  const std::string address(msg_view.begin(), msg_view.begin() + n);
  if (!EthAddress::IsValidAddress(address)) {
    return false;
  }
  address_out = address;
  CHECK(ShrinkMessage(msg_view, n + 1));
  state_ = State::kStatement;
  return true;
}

bool SIWEMessageParser::ParseStatement(std::string_view& msg_view,
                                       std::optional<std::string>& statement) {
  if (msg_view.size() < 2 || msg_view[0] != '\n') {
    return false;
  }
  if (msg_view[1] == '\n') {
    // jump forward "\n\n"
    CHECK(ShrinkMessage(msg_view, 2));
    state_ = State::kURI;
    // no statement
    return true;
  }
  std::string::size_type stmt_lf = msg_view.find('\n', 1);
  if (stmt_lf == std::string::npos) {
    return false;
  }
  // closing lf
  if (stmt_lf + 1 >= msg_view.length() || msg_view[stmt_lf + 1] != '\n') {
    return false;
  }
  statement = std::string(msg_view.begin() + 1, msg_view.begin() + stmt_lf);
  if (!base::IsStringASCII(*statement)) {
    return false;
  }
  CHECK(ShrinkMessage(msg_view, stmt_lf + 2));
  state_ = State::kURI;
  return true;
}

bool SIWEMessageParser::ParseURI(std::string_view& msg_view, GURL& uri_out) {
  auto value = ExtractValue(msg_view, kURIToken);
  if (!value) {
    return false;
  }

  GURL uri(value->second);
  if (!uri.is_valid()) {
    return false;
  }
  uri_out = uri;
  CHECK(ShrinkMessage(msg_view, value->first + 1));
  state_ = State::kVersion;
  return true;
}

bool SIWEMessageParser::ParseVersion(std::string_view& msg_view,
                                     uint32_t& version) {
  auto value = ExtractValue(msg_view, kVersionToken);
  if (!value) {
    return false;
  }
  if (!base::StringToUint(value->second, &version)) {
    return false;
  }
  // The only supported version should be 1
  if (version != 1) {
    return false;
  }

  CHECK(ShrinkMessage(msg_view, value->first + 1));
  state_ = State::kChainId;
  return true;
}

bool SIWEMessageParser::ParseChainId(std::string_view& msg_view,
                                     uint64_t& chain_id) {
  auto value = ExtractValue(msg_view, kChainIdToken);
  if (!value) {
    return false;
  }
  if (!base::StringToUint64(value->second, &chain_id)) {
    return false;
  }

  CHECK(ShrinkMessage(msg_view, value->first + 1));
  state_ = State::kNonce;
  return true;
}

bool SIWEMessageParser::ParseNonce(std::string_view& msg_view,
                                   std::string& nonce) {
  auto value = ExtractValue(msg_view, kNonceToken);
  if (!value) {
    return false;
  }
  nonce = value->second;
  if (nonce.size() < kMinNonceLength ||
      !base::ranges::all_of(nonce, &base::IsAsciiAlphaNumeric<char>)) {
    return false;
  }

  CHECK(ShrinkMessage(msg_view, value->first + 1));
  state_ = State::kIssuedAt;
  return true;
}

bool SIWEMessageParser::ParseIssuedAt(std::string_view& msg_view,
                                      std::string& issued_at) {
  // IssuedAt is the last required field so we have to check if it contains
  // following optional fields
  bool has_optional_fields = msg_view.rfind('\n') != std::string::npos;
  auto value = ExtractValue(msg_view, kIssuedAtToken, has_optional_fields);
  if (!value) {
    return false;
  }
  issued_at = value->second;
  CHECK(ShrinkMessage(msg_view, value->first));
  if (has_optional_fields) {
    state_ = State::kOptionalFields;
  } else {
    state_ = State::kEnd;
  }
  return true;
}

bool SIWEMessageParser::ParseOptionalStringField(
    std::string_view& msg_view,
    const std::string& name,
    std::optional<std::string>& value_out) {
  auto value = ExtractOptionalValue(msg_view, name);
  if (!value) {
    return false;
  }
  // Not presented
  if (value->first == std::string::npos && value->second == msg_view) {
    return true;
  }
  value_out = value->second;
  CHECK(ShrinkMessage(msg_view, value->first));
  return true;
}

bool SIWEMessageParser::ParseOptionalResources(
    std::string_view& msg_view,
    std::optional<std::vector<GURL>>& resources) {
  if (!msg_view.empty() && msg_view.back() == '\n') {
    return false;
  }
  auto value = ExtractOptionalValue(msg_view, kResourcesToken, true);
  if (!value) {
    return false;
  }
  // Not presented
  if (value->first == std::string::npos && value->second == msg_view) {
    return true;
  }
  const std::string urls_str(value->second);
  StringTokenizer tokenizer(urls_str, "\n");
  std::vector<GURL> urls;
  while (tokenizer.GetNext()) {
    if (!tokenizer.token().starts_with(kResourcesSeperator)) {
      return false;
    }
    auto url_str = tokenizer.token().substr(strlen(kResourcesSeperator));
    GURL url(url_str);
    if (!url.is_valid()) {
      return false;
    }
    urls.push_back(url);
  }
  if (!urls.size()) {
    return false;
  }
  resources = urls;
  CHECK(ShrinkMessage(msg_view, value->first));
  return true;
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

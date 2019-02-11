/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "unblinded_tokens.h"

#include "base/logging.h"

namespace confirmations {

UnblindedTokens::UnblindedTokens(ConfirmationsImpl* confirmations) :
    confirmations_(confirmations) {
}

UnblindedTokens::~UnblindedTokens() = default;

UnblindedToken UnblindedTokens::GetToken() const {
  DCHECK_NE(Count(), 0);
  return unblinded_tokens_.front();
}

std::vector<UnblindedToken> UnblindedTokens::GetAllTokens() const {
  return unblinded_tokens_;
}

base::Value UnblindedTokens::GetTokensAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& token : unblinded_tokens_) {
    auto token_base64 = token.encode_base64();
    auto token_value = base::Value(token_base64);
    list.GetList().push_back(std::move(token_value));
  }

  return list;
}

void UnblindedTokens::SetTokens(const std::vector<UnblindedToken>& tokens) {
  unblinded_tokens_ = tokens;

  confirmations_->SaveState();
}

void UnblindedTokens::SetTokensFromList(const base::ListValue& list) {
  std::vector<UnblindedToken> unblinded_tokens;
  for (const auto& token_value : list) {
    auto token_base64 = token_value.GetString();
    auto token = UnblindedToken::decode_base64(token_base64);
    unblinded_tokens.push_back(token);
  }

  SetTokens(unblinded_tokens);
}

void UnblindedTokens::AddTokens(const std::vector<UnblindedToken>& tokens) {
  unblinded_tokens_.insert(unblinded_tokens_.end(), tokens.begin(),
      tokens.end());

  confirmations_->SaveState();
}

bool UnblindedTokens::RemoveToken(const UnblindedToken& token) {
  auto it = std::find(unblinded_tokens_.begin(), unblinded_tokens_.end(),
      token);

  if (it == unblinded_tokens_.end()) {
    return false;
  }

  unblinded_tokens_.erase(it);

  confirmations_->SaveState();

  return true;
}

void UnblindedTokens::RemoveAllTokens() {
  unblinded_tokens_.clear();

  confirmations_->SaveState();
}

bool UnblindedTokens::TokenExists(const UnblindedToken& token) {
  auto it = std::find(unblinded_tokens_.begin(), unblinded_tokens_.end(),
      token);

  if (it == unblinded_tokens_.end()) {
    return false;
  }

  return true;
}

int UnblindedTokens::Count() const {
  return unblinded_tokens_.size();
}

bool UnblindedTokens::IsEmpty() const {
  if (Count() > 0) {
    return false;
  }

  return true;
}

}  // namespace confirmations

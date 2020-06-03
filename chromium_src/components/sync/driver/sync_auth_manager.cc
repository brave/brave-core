/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/sync/driver/sync_auth_manager.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "components/signin/public/identity_manager/accounts_mutator.h"
#include "components/signin/public/identity_manager/primary_account_mutator.h"

#define BRAVE_ACCESS_TOKEN_FETCHED                                \
  if (!access_token_info.token.empty() && !public_key_.empty() && \
      !private_key_.empty())                                      \
    access_token_ = GenerateAccessToken(access_token_info.token);

#include "../../../../../components/sync/driver/sync_auth_manager.cc"

#undef BRAVE_ACCESS_TOKEN_FETCHED

namespace syncer {

void SyncAuthManager::DeriveSigningKeys(const std::string& seed) {
  VLOG(1) << __func__ << " seed=" << seed;
  if (seed.empty())
    return;
  const std::vector<uint8_t> HKDF_SALT = {
      72,  203, 156, 43,  64,  229, 225, 127, 214, 158, 50,  29,  130,
      186, 182, 207, 6,   108, 47,  254, 245, 71,  198, 109, 44,  108,
      32,  193, 221, 126, 119, 143, 112, 113, 87,  184, 239, 231, 230,
      234, 28,  135, 54,  42,  9,   243, 39,  30,  179, 147, 194, 211,
      212, 239, 225, 52,  192, 219, 145, 40,  95,  19,  142, 98};
  std::vector<uint8_t> seed_bytes;
  brave_sync::crypto::PassphraseToBytes32(seed, &seed_bytes);
  brave_sync::crypto::DeriveSigningKeysFromSeed(seed_bytes, &HKDF_SALT,
                                                &public_key_, &private_key_);
  const std::string gaia_id =
      base::HexEncode(public_key_.data(), public_key_.size());
  const std::string email = "sync@brave.com";
  if (!identity_manager_->HasPrimaryAccount()) {
    const CoreAccountId account_id =
        identity_manager_->GetAccountsMutator()->AddOrUpdateAccount(
            gaia_id, email, "dummy_refresh_token", true,
            signin_metrics::SourceForRefreshTokenOperation::
                kInlineLoginHandler_Signin);
    auto* primary_account_mutator =
        identity_manager_->GetPrimaryAccountMutator();
    primary_account_mutator->SetPrimaryAccount(account_id);
  }
  VLOG(1) << "account_id="
          << identity_manager_->GetPrimaryAccountId().ToString();
}

void SyncAuthManager::ResetKeys() {
  VLOG(1) << __func__;
  public_key_.clear();
  private_key_.clear();
  // Signout will be handled in PeopleHandler::CloseSyncSetup()
}

// OAuth2AccessTokenFetcherImpl is responsible for fetching timestamp back and
// we compose the access token here
std::string SyncAuthManager::GenerateAccessToken(const std::string& timestamp) {
  DCHECK(!timestamp.empty() && !public_key_.empty() && !private_key_.empty());
  const std::string public_key_hex =
      base::HexEncode(public_key_.data(), public_key_.size());

  const std::string timestamp_hex =
      base::HexEncode(timestamp.data(), timestamp.size());

  std::vector<uint8_t> timestamp_bytes;
  base::HexStringToBytes(timestamp_hex, &timestamp_bytes);
  std::vector<uint8_t> signature;
  brave_sync::crypto::Sign(timestamp_bytes, private_key_, &signature);
  DCHECK(brave_sync::crypto::Verify(timestamp_bytes, signature, public_key_));

  const std::string signed_timestamp_hex =
      base::HexEncode(signature.data(), signature.size());

  // base64(timestamp_hex|signed_timestamp_hex|public_key_hex)
  const std::string access_token =
      timestamp_hex + "|" + signed_timestamp_hex + "|" + public_key_hex;
  std::string encoded_access_token;
  base::Base64Encode(access_token, &encoded_access_token);
  DCHECK(!encoded_access_token.empty());

  VLOG(1) << "access_token= " << encoded_access_token;
  return encoded_access_token;
}

}  // namespace syncer

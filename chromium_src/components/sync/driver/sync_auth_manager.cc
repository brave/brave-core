#include "brave/chromium_src/components/sync/driver/sync_auth_manager.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/crypto/crypto.h"

#define BRAVE_REQUEST_ACCESS_TOKEN                        \
  VLOG(1) << __func__;                                    \
  if (private_key_.empty() || public_key_.empty()) {      \
    request_access_token_backoff_.InformOfRequest(false); \
    ScheduleAccessTokenRequest();                         \
    return;                                               \
  }                                                       \
  access_token_fetcher_->StartGetTimestamp();

#define BRAVE_DETERMINE_ACCOUNT_TO_USE                                     \
  AccountInfo account_info;                                                \
  account_info.account_id = CoreAccountId::FromString("dummy_account_id"); \
  account_info.email = "dummy@brave.com";                                  \
  SyncAccountInfo account(account_info, true);                             \
  return account;
#include "../../../../../components/sync/driver/sync_auth_manager.cc"
#undef BRAVE_REQUEST_ACCESS_TOKEN
#undef BRAVE_DETERMINE_ACCOUNT_TO_USE

namespace syncer {

void SyncAuthManager::CreateAccessTokenFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const GURL& sync_service_url) {
  access_token_fetcher_ = std::make_unique<brave_sync::AccessTokenFetcherImpl>(
      this, url_loader_factory, sync_service_url, "");
}

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
}

void SyncAuthManager::ResetKeys() {
  VLOG(1) << __func__;
  public_key_.clear();
  private_key_.clear();
}

void SyncAuthManager::OnGetTokenSuccess(
    const brave_sync::AccessTokenConsumer::TokenResponse& token_response) {
  AccessTokenFetched(GoogleServiceAuthError(GoogleServiceAuthError::NONE),
                     signin::AccessTokenInfo(token_response.access_token,
                                             token_response.expiration_time,
                                             token_response.id_token));
  VLOG(1) << __func__ << " Token: " << access_token_;
}

void SyncAuthManager::OnGetTokenFailure(const GoogleServiceAuthError& error) {
  LOG(ERROR) << __func__ << ": " << error.error_message();
  AccessTokenFetched(error, signin::AccessTokenInfo());
}

void SyncAuthManager::OnGetTimestampSuccess(const std::string& ts) {
  VLOG(1) << __func__ << " Timestamp: " << ts;
  std::string client_id, client_secret, timestamp;
  GenerateClientIdAndSecret(&client_id, &client_secret, ts, &timestamp);
  access_token_fetcher_->Start(client_id, client_secret, timestamp);
}

void SyncAuthManager::OnGetTimestampFailure(
    const GoogleServiceAuthError& error) {
  LOG(ERROR) << __func__ << ": " << error.error_message();
  AccessTokenFetched(error, signin::AccessTokenInfo());
}

void SyncAuthManager::GenerateClientIdAndSecret(
    std::string* client_id,
    std::string* client_secret,
    const std::string& server_timestamp,
    std::string* timestamp) {
  DCHECK(client_id);
  DCHECK(client_secret);

  *client_id = base::HexEncode(public_key_.data(), public_key_.size());

  *timestamp =
      base::HexEncode(server_timestamp.data(), server_timestamp.size());

  std::vector<uint8_t> timestamp_bytes;
  base::HexStringToBytes(*timestamp, &timestamp_bytes);
  std::vector<uint8_t> signature;
  brave_sync::crypto::Sign(timestamp_bytes, private_key_, &signature);
  DCHECK(brave_sync::crypto::Verify(timestamp_bytes, signature, public_key_));

  *client_secret = base::HexEncode(signature.data(), signature.size());

  VLOG(1) << "client_id= " << *client_id;
  VLOG(1) << "client_secret= " << *client_secret;
  VLOG(1) << "timestamp= " << *timestamp;
}

}  // namespace syncer

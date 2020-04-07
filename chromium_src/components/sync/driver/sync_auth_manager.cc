#include "brave/chromium_src/components/sync/driver/sync_auth_manager.h"

#define BRAVE_REQUEST_ACCESS_TOKEN \
  access_token_fetcher_->Start("ID", "SECRECT");
#include "../../../../../components/sync/driver/sync_auth_manager.cc"
#undef BRAVE_REQUEST_ACCESS_TOKEN

namespace syncer {

void SyncAuthManager::CreateAccessTokenFetcher(                                            
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  access_token_fetcher_ = std::make_unique<brave_sync::AccessTokenFetcherImpl>(
  this, url_loader_factory, "");
}
void SyncAuthManager::OnGetTokenSuccess(                                                   
      const brave_sync::AccessTokenConsumer::TokenResponse& token_response) {
    access_token_ = token_response.access_token;
    VLOG(1) << "Got Token: " << access_token_;
    SetLastAuthError(GoogleServiceAuthError::AuthErrorNone());
}
void SyncAuthManager::OnGetTokenFailure(const std::string& error) {
  LOG(ERROR) << __func__ << ": " << error;
  // TODO(darkdh): SetLastAuthError
  request_access_token_backoff_.InformOfRequest(false);
  ScheduleAccessTokenRequest();
}         

}  // namespace syncer

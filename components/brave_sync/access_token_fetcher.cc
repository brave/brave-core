#include "brave/components/brave_sync/access_token_fetcher.h"

namespace brave_sync {

AccessTokenFetcher::AccessTokenFetcher(AccessTokenConsumer* consumer)
    : consumer_(consumer) {}

AccessTokenFetcher::~AccessTokenFetcher() {}

void AccessTokenFetcher::FireOnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response) {
  consumer_->OnGetTokenSuccess(token_response);
}

void AccessTokenFetcher::FireOnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  consumer_->OnGetTokenFailure(error);
}

void AccessTokenFetcher::FireOnGetTimestampSuccess(const std::string& ts) {
  consumer_->OnGetTimestampSuccess(ts);
}

void AccessTokenFetcher::FireOnGetTimestampFailure(
    const GoogleServiceAuthError& error) {
  consumer_->OnGetTimestampFailure(error);
}

}  // namespace brave_sync

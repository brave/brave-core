#include "brave/components/brave_sync/access_token_consumer.h"

namespace brave_sync {

AccessTokenConsumer::TokenResponse::TokenResponse(
    const std::string& access_token,
    const base::Time& expiration_time,
    const std::string& id_token)
    : access_token(access_token),
      expiration_time(expiration_time),
      id_token(id_token) {}

AccessTokenConsumer::~AccessTokenConsumer() {}

void AccessTokenConsumer::OnGetTokenSuccess(
    const TokenResponse& token_response) {}

void AccessTokenConsumer::OnGetTokenFailure(
    const std::string& error) {}

void AccessTokenConsumer::OnGetTimestampSuccess(const std::string& ts) {}

void AccessTokenConsumer::OnGetTimestampFailure(const std::string& error) {}

}   // namespace brave_sync
